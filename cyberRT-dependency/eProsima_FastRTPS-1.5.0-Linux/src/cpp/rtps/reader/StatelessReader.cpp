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
 * @file StatelessReader.cpp
 *             	
 */

#include <fastrtps/rtps/reader/StatelessReader.h>
#include <fastrtps/rtps/history/ReaderHistory.h>
#include <fastrtps/rtps/reader/ReaderListener.h>
#include <fastrtps/log/Log.h>
#include <fastrtps/rtps/common/CacheChange.h>
#include "../participant/RTPSParticipantImpl.h"
#include "FragmentedChangePitStop.h"


#include <mutex>
#include <thread>

#include <cassert>

#define IDSTRING "(ID:"<< std::this_thread::get_id() <<") "<<

using namespace eprosima::fastrtps::rtps;


StatelessReader::~StatelessReader()
{
    logInfo(RTPS_READER,"Removing reader "<<this->getGuid());
}

StatelessReader::StatelessReader(RTPSParticipantImpl* pimpl,GUID_t& guid,
        ReaderAttributes& att,ReaderHistory* hist,ReaderListener* listen):
    RTPSReader(pimpl,guid,att,hist, listen)
{

}



bool StatelessReader::matched_writer_add(RemoteWriterAttributes& wdata)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    for(auto it = m_matched_writers.begin();it!=m_matched_writers.end();++it)
    {
        if((*it).guid == wdata.guid)
            return false;
    }
    logInfo(RTPS_READER,"Writer " << wdata.guid << " added to "<<m_guid.entityId);
    m_matched_writers.push_back(wdata);
    m_acceptMessagesFromUnkownWriters = false;
    return true;
}
bool StatelessReader::matched_writer_remove(RemoteWriterAttributes& wdata)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    for(auto it = m_matched_writers.begin();it!=m_matched_writers.end();++it)
    {
        if((*it).guid == wdata.guid)
        {
            logInfo(RTPS_READER,"Writer " <<wdata.guid<< " removed from "<<m_guid.entityId);
            m_matched_writers.erase(it);
            m_historyRecord.erase(wdata.guid);
            return true;
        }
    }
    return false;
}

bool StatelessReader::matched_writer_is_matched(RemoteWriterAttributes& wdata)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    for(auto it = m_matched_writers.begin();it!=m_matched_writers.end();++it)
    {
        if((*it).guid == wdata.guid)
        {
            return true;
        }
    }
    return false;
}

bool StatelessReader::change_received(CacheChange_t* change, std::unique_lock<std::recursive_mutex> &lock)
{
    // Only make visible the change if there is not other with bigger sequence number.
    // TODO Revisar si no hay que incluirlo.
    if(!thereIsUpperRecordOf(change->writerGUID, change->sequenceNumber))
    {
        if(mp_history->received_change(change, 0))
        {
            m_historyRecord[change->writerGUID] = change->sequenceNumber;

            if(getListener() != nullptr)
            {
                lock.unlock();
                getListener()->onNewCacheChangeAdded((RTPSReader*)this,change);
                lock.lock();
            }

            mp_history->postSemaphore();
            return true;
        }
    }

    return false;
}

bool StatelessReader::nextUntakenCache(CacheChange_t** change, WriterProxy** /*wpout*/)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    return mp_history->get_min_change(change);
}


bool StatelessReader::nextUnreadCache(CacheChange_t** change,WriterProxy** /*wpout*/)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    //m_reader_cache.sortCacheChangesBySeqNum();
    bool found = false;
    std::vector<CacheChange_t*>::iterator it;
    //TODO PROTEGER ACCESO A HISTORIA AQUI??? YO CREO QUE NO, YA ESTA EL READER PROTEGIDO
    for(it = mp_history->changesBegin();
            it!=mp_history->changesEnd();++it)
    {
        if(!(*it)->isRead)
        {
            found = true;
            break;
        }
    }
    if(found)
    {
        *change = *it;
        return true;
    }
    logInfo(RTPS_READER,"No Unread elements left");
    return false;
}


bool StatelessReader::change_removed_by_history(CacheChange_t* /*ch*/, WriterProxy* /*prox*/)
{
    return true;
}

bool StatelessReader::processDataMsg(CacheChange_t *change)
{

    assert(change);

    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    if(acceptMsgFrom(change->writerGUID))
    {
        logInfo(RTPS_MSG_IN,IDSTRING"Trying to add change " << change->sequenceNumber <<" TO reader: "<< getGuid().entityId);

        CacheChange_t* change_to_add;
        if(reserveCache(&change_to_add, change->serializedPayload.length)) //Reserve a new cache from the corresponding cache pool
        {
#if HAVE_SECURITY
            if(is_payload_protected())
            {
                change_to_add->copy_not_memcpy(change);
                if(!getRTPSParticipant()->security_manager().decode_serialized_payload(change->serializedPayload,
                        change_to_add->serializedPayload, m_guid, change->writerGUID))
                {
                    releaseCache(change_to_add);
                    logWarning(RTPS_MSG_IN, "Cannont decode serialized payload");
                    return false;
                }
            }
            else
            {
#endif
                if (!change_to_add->copy(change))
                {
                    logWarning(RTPS_MSG_IN,IDSTRING"Problem copying CacheChange, received data is: " << change->serializedPayload.length
                            << " bytes and max size in reader " << getGuid().entityId << " is " << change_to_add->serializedPayload.max_size);
                    releaseCache(change_to_add);
                    return false;
                }
#if HAVE_SECURITY
            }
#endif
        }
        else
        {
            logError(RTPS_MSG_IN,IDSTRING"Problem reserving CacheChange in reader: " << getGuid().entityId);
            return false;
        }

        if(!change_received(change_to_add, lock))
        {
            logInfo(RTPS_MSG_IN,IDSTRING"MessageReceiver not add change "
                    <<change_to_add->sequenceNumber);
            releaseCache(change_to_add);

            if(getGuid().entityId == c_EntityId_SPDPReader)
            {
                mp_RTPSParticipant->assertRemoteRTPSParticipantLiveliness(change->writerGUID.guidPrefix);
            }
        }
    }

    return true;
}

bool StatelessReader::processDataFragMsg(CacheChange_t *incomingChange, uint32_t sampleSize, uint32_t fragmentStartingNum)
{
    assert(incomingChange);

    std::unique_lock<std::recursive_mutex> lock(*mp_mutex);

    if (acceptMsgFrom(incomingChange->writerGUID))
    {
        // Check if CacheChange was received.
        if(!thereIsUpperRecordOf(incomingChange->writerGUID, incomingChange->sequenceNumber))
        {
            logInfo(RTPS_MSG_IN, IDSTRING"Trying to add fragment " << incomingChange->sequenceNumber.to64long() << " TO reader: " << getGuid().entityId);

            // Fragments manager has to process incomming fragments.
            // If CacheChange_t is completed, it will be returned;
            CacheChange_t* change_completed = fragmentedChangePitStop_->process(incomingChange, sampleSize, fragmentStartingNum);

            // Try to remove previous CacheChange_t from PitStop.
            fragmentedChangePitStop_->try_to_remove_until(incomingChange->sequenceNumber, incomingChange->writerGUID);

            // If the change was completed, process it.
            if(change_completed != nullptr)
            {

                CacheChange_t* change_to_add = nullptr;

#if HAVE_SECURITY
                if(is_payload_protected())
                {
                    if(reserveCache(&change_to_add, change_completed->serializedPayload.length)) //Reserve a new cache from the corresponding cache pool
                    {
                        change_to_add->copy_not_memcpy(change_completed);
                        if(!getRTPSParticipant()->security_manager().decode_serialized_payload(change_completed->serializedPayload,
                                    change_to_add->serializedPayload, m_guid, change_completed->writerGUID))
                        {
                            releaseCache(change_to_add);
                            releaseCache(change_completed);
                            logWarning(RTPS_MSG_IN, "Cannont decode serialized payload");
                            return false;
                        }
                    }

                    releaseCache(change_completed);
                }
                else
                {
#endif
                    change_to_add = change_completed;
#if HAVE_SECURITY
                }
#endif

                if (!change_received(change_to_add, lock))
                {
                    logInfo(RTPS_MSG_IN, IDSTRING"MessageReceiver not add change " << change_to_add->sequenceNumber.to64long());

                    // Assert liveliness because if it is a participant discovery info.
                    if (getGuid().entityId == c_EntityId_SPDPReader)
                    {
                        mp_RTPSParticipant->assertRemoteRTPSParticipantLiveliness(incomingChange->writerGUID.guidPrefix);
                    }

                    // Release CacheChange_t.
                    releaseCache(change_to_add);
                }
            }
        }
    }

    return true;
}

bool StatelessReader::processHeartbeatMsg(GUID_t& /*writerGUID*/, uint32_t /*hbCount*/, SequenceNumber_t& /*firstSN*/,
        SequenceNumber_t& /*lastSN*/, bool /*finalFlag*/, bool /*livelinessFlag*/)
{
    return true;
}

bool StatelessReader::processGapMsg(GUID_t& /*writerGUID*/, SequenceNumber_t& /*gapStart*/, SequenceNumberSet_t& /*gapList*/)
{
    return true;
}

bool StatelessReader::acceptMsgFrom(GUID_t& writerId)
{
    if(this->m_acceptMessagesFromUnkownWriters)
    {
        return true;
    }
    else
    {
        if(writerId.entityId == this->m_trustedWriterEntityId)
            return true;

        for(auto it = m_matched_writers.begin();it!=m_matched_writers.end();++it)
        {
            if((*it).guid == writerId)
            {
                return true;
            }
        }
    }

    return false;
}

bool StatelessReader::thereIsUpperRecordOf(GUID_t& guid, SequenceNumber_t& seq)
{
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    return m_historyRecord.find(guid) != m_historyRecord.end() && m_historyRecord[guid] >= seq;
}
