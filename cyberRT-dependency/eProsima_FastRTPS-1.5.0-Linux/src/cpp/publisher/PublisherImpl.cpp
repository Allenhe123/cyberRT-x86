// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * Publisher.cpp
 *
 */

#include "PublisherImpl.h"
#include "../participant/ParticipantImpl.h"
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/TopicDataType.h>
#include <fastrtps/publisher/PublisherListener.h>

#include <fastrtps/rtps/writer/RTPSWriter.h>
#include <fastrtps/rtps/writer/StatefulWriter.h>

#include <fastrtps/rtps/participant/RTPSParticipant.h>
#include <fastrtps/rtps/RTPSDomain.h>

#include <fastrtps/log/Log.h>

using namespace eprosima::fastrtps;
using namespace ::rtps;



::rtps::WriteParams WRITE_PARAM_DEFAULT;

PublisherImpl::PublisherImpl(ParticipantImpl* p,TopicDataType*pdatatype,
        PublisherAttributes& att,PublisherListener* listen ):
    mp_participant(p),
    mp_writer(nullptr),
    mp_type(pdatatype),
    m_att(att),
#pragma warning (disable : 4355 )
    m_history(this, pdatatype->m_typeSize, att.topic.historyQos, att.topic.resourceLimitsQos, att.historyMemoryPolicy),
    mp_listener(listen),
#pragma warning (disable : 4355 )
    m_writerListener(this),
    mp_userPublisher(nullptr),
    mp_rtpsParticipant(nullptr),
    high_mark_for_frag_(0)
{
}

PublisherImpl::~PublisherImpl()
{
    if(mp_writer != nullptr)
    {
        logInfo(PUBLISHER, this->getGuid().entityId << " in topic: " << this->m_att.topic.topicName);
    }

    RTPSDomain::removeRTPSWriter(mp_writer);
    delete(this->mp_userPublisher);
}



bool PublisherImpl::create_new_change(ChangeKind_t changeKind, void* data)
{
    return create_new_change_with_params(changeKind, data, WRITE_PARAM_DEFAULT);
}

bool PublisherImpl::create_new_change_with_params(ChangeKind_t changeKind, void* data, WriteParams &wparams)
{

    /// Preconditions
    if (data == nullptr)
    {
        logError(PUBLISHER, "Data pointer not valid");
        return false;
    }

    if(changeKind == NOT_ALIVE_UNREGISTERED || changeKind == NOT_ALIVE_DISPOSED ||
            changeKind == NOT_ALIVE_DISPOSED_UNREGISTERED)
    {
        if(m_att.topic.topicKind == NO_KEY)
        {
            logError(PUBLISHER,"Topic is NO_KEY, operation not permitted");
            return false;
        }
    }

    InstanceHandle_t handle;
    if(m_att.topic.topicKind == WITH_KEY)
    {
        mp_type->getKey(data,&handle);
    }

    // Block lowlevel writer
    std::unique_lock<std::recursive_mutex> lock(*mp_writer->getMutex());

    CacheChange_t* ch = mp_writer->new_change(mp_type->getSerializedSizeProvider(data), changeKind, handle);
    if(ch != nullptr)
    {
        if(changeKind == ALIVE)
        {
            //If these two checks are correct, we asume the cachechange is valid and thwn we can write to it.
            if(!mp_type->serialize(data, &ch->serializedPayload))
            {
                logWarning(RTPS_WRITER,"RTPSWriter:Serialization returns false";);
                m_history.release_Cache(ch);
                return false;
            }
        }

        //TODO(Ricardo) This logic in a class. Then a user of rtps layer can use it.
        if(high_mark_for_frag_ == 0)
        {
            uint32_t max_data_size = mp_writer->getMaxDataSize();
            uint32_t writer_throughput_controller_bytes =
                mp_writer->calculateMaxDataSize(m_att.throughputController.bytesPerPeriod);
            uint32_t participant_throughput_controller_bytes =
                mp_writer->calculateMaxDataSize(mp_rtpsParticipant->getRTPSParticipantAttributes().throughputController.bytesPerPeriod);

            high_mark_for_frag_ =
                max_data_size > writer_throughput_controller_bytes ?
                writer_throughput_controller_bytes :
                (max_data_size > participant_throughput_controller_bytes ?
                 participant_throughput_controller_bytes :
                 max_data_size);
        }

        // If it is big data, fragment it.
        if(ch->serializedPayload.length > high_mark_for_frag_)
        {
            // Check ASYNCHRONOUS_PUBLISH_MODE is being used, but it is an error case.
            if( m_att.qos.m_publishMode.kind != ASYNCHRONOUS_PUBLISH_MODE)
            {
                logError(PUBLISHER, "Data cannot be sent. It's serialized size is " <<
                        ch->serializedPayload.length << "' which exceeds the maximum payload size of '" <<
                        high_mark_for_frag_ << "' and therefore ASYNCHRONOUS_PUBLISH_MODE must be used.");
                m_history.release_Cache(ch);
                return false;
            }

            /// Fragment the data.
            // Set the fragment size to the cachechange.
            // Note: high_mark will always be a value that can be casted to uint16_t)
            ch->setFragmentSize((uint16_t)high_mark_for_frag_);
        }

        if(&wparams != &WRITE_PARAM_DEFAULT)
        {
            ch->write_params = wparams;
        }

        if(!this->m_history.add_pub_change(ch, wparams))
        {
            m_history.release_Cache(ch);
            return false;
        }

        return true;
    }

    return false;
}


bool PublisherImpl::removeMinSeqChange()
{
    return m_history.removeMinChange();
}

bool PublisherImpl::removeAllChange(size_t* removed)
{
    return m_history.removeAllChange(removed);
}

const GUID_t& PublisherImpl::getGuid()
{
    return mp_writer->getGuid();
}
//
bool PublisherImpl::updateAttributes(PublisherAttributes& att)
{
    bool updated = true;
    bool missing = false;
    if(this->m_att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS)
    {
        if(att.unicastLocatorList.size() != this->m_att.unicastLocatorList.size() ||
                att.multicastLocatorList.size() != this->m_att.multicastLocatorList.size())
        {
            logWarning(PUBLISHER,"Locator Lists cannot be changed or updated in this version");
            updated &= false;
        }
        else
        {
            for(LocatorListIterator lit1 = this->m_att.unicastLocatorList.begin();
                    lit1!=this->m_att.unicastLocatorList.end();++lit1)
            {
                missing = true;
                for(LocatorListIterator lit2 = att.unicastLocatorList.begin();
                        lit2!= att.unicastLocatorList.end();++lit2)
                {
                    if(*lit1 == *lit2)
                    {
                        missing = false;
                        break;
                    }
                }
                if(missing)
                {
                    logWarning(PUBLISHER,"Locator: "<< *lit1 << " not present in new list");
                    logWarning(PUBLISHER,"Locator Lists cannot be changed or updated in this version");
                }
            }
            for(LocatorListIterator lit1 = this->m_att.multicastLocatorList.begin();
                    lit1!=this->m_att.multicastLocatorList.end();++lit1)
            {
                missing = true;
                for(LocatorListIterator lit2 = att.multicastLocatorList.begin();
                        lit2!= att.multicastLocatorList.end();++lit2)
                {
                    if(*lit1 == *lit2)
                    {
                        missing = false;
                        break;
                    }
                }
                if(missing)
                {
                    logWarning(PUBLISHER,"Locator: "<< *lit1<< " not present in new list");
                    logWarning(PUBLISHER,"Locator Lists cannot be changed or updated in this version");
                }
            }
        }
    }

    //TOPIC ATTRIBUTES
    if(this->m_att.topic != att.topic)
    {
        logWarning(PUBLISHER,"Topic Attributes cannot be updated");
        updated &= false;
    }
    //QOS:
    //CHECK IF THE QOS CAN BE SET
    if(!this->m_att.qos.canQosBeUpdated(att.qos))
    {
        updated &=false;
    }
    if(updated)
    {
        if(this->m_att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS)
        {
            //UPDATE TIMES:
            StatefulWriter* sfw = (StatefulWriter*)mp_writer;
            sfw->updateTimes(att.times);
        }
        this->m_att.qos.setQos(att.qos,false);
        this->m_att = att;
        //Notify the participant that a Writer has changed its QOS
        mp_rtpsParticipant->updateWriter(this->mp_writer,m_att.qos);
    }


    return updated;
}

void PublisherImpl::PublisherWriterListener::onWriterMatched(RTPSWriter* /*writer*/,MatchingInfo& info)
{
    if(mp_publisherImpl->mp_listener!=nullptr)
        mp_publisherImpl->mp_listener->onPublicationMatched(mp_publisherImpl->mp_userPublisher,info);
}

bool PublisherImpl::clean_history(unsigned int max)
{
    if(m_att.topic.historyQos.kind == HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS)
        return mp_writer->clean_history(max);
    else
        return mp_writer->remove_older_changes(max);
}

bool PublisherImpl::wait_for_all_acked(const Time_t& max_wait)
{
    return mp_writer->wait_for_all_acked(max_wait);
}
