/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Handles messages!
* Version:      1.0
* Created:      09/09/2008 12:14:35 PM
* Revision:     none
* Compiler:     gcc
* Author:       David Irvine (di), david.irvine@maidsafe.net
* Company:      maidsafe.net limited
*
* The following source code is property of maidsafe.net limited and is not
* meant for external use.  The use of this code is governed by the license
* file LICENSE.TXT found in the root of this directory and also on
* www.maidsafe.net.
*
* You are not free to copy, amend or otherwise use this source code without
* the explicit written permission of the board of directors of maidsafe.net.
*
* ============================================================================
*/

#include "maidsafe/client/messagehandler.h"

#include <stdlib.h>
#include <time.h>

#include <cstdio>
#include <list>

#include "protobuf/packet.pb.h"
#include "protobuf/maidsafe_service_messages.pb.h"

namespace maidsafe {

MessageHandler::MessageHandler(StoreManagerInterface *sm,
                               boost::recursive_mutex *mutex)
                                   : ss_(SessionSingleton::getInstance()),
                                     sm_(sm),
                                     co_(),
                                     mutex_(mutex) {
  co_.set_hash_algorithm(crypto::SHA_512);
  co_.set_symm_algorithm(crypto::AES_256);
}

void MessageHandler::SendMessage(const std::string &msg,
                                 const std::vector<Receivers> &receivers,
                                 const BufferPacketType &p_type,
                                 const packethandler::MessageType &m_type,
                                 base::callback_func_type cb,
                                 const boost::uint32_t &timestamp) {
  base::pd_scoped_lock gaurd(*mutex_);
  boost::shared_ptr<SendMessagesData> data(new SendMessagesData());
  data->index = -1;
  data->is_calledback = false;
  data->cb = cb;
  data->active_sends = 0;
  data->receivers = receivers;
  std::vector<std::string> no_auth_rec;
  data->no_auth_rec = no_auth_rec;
  data->successful_stores = 0;
  data->stores_done = 0;
  data->p_type = p_type;
  data->m_type = m_type;
  data->msg = msg;
  data->timestamp = timestamp;
  IterativeStoreMsgs(data);
}

std::string MessageHandler::CreateMessage(
    const std::string &msg,
    const std::string &rec_public_key,
    const packethandler::MessageType &m_type,
    const BufferPacketType &p_type,
    const boost::uint32_t &timestamp) {
  maidsafe::PacketType pt = PacketHandler_PacketType(p_type);
  packethandler::BufferPacketMessage bpm;
  packethandler::GenericPacket gp;

  bpm.set_sender_id(ss_->Id(pt));
  bpm.set_sender_public_key(ss_->PublicKey(pt));
  bpm.set_type(m_type);
  int iter = base::random_32bit_uinteger() % 1000 +1;
  std::string aes_key = co_.SecurePassword(
      co_.Hash(msg, "", crypto::STRING_STRING, true),
      iter);
  bpm.set_rsaenc_key(co_.AsymEncrypt(aes_key,
                                     "",
                                     rec_public_key,
                                     crypto::STRING_STRING));
  bpm.set_aesenc_message(co_.SymmEncrypt(msg,
                                         "",
                                         crypto::STRING_STRING,
                                         aes_key));
  bpm.set_timestamp(timestamp);
  std::string ser_bpm;
  bpm.SerializeToString(&ser_bpm);
  gp.set_data(ser_bpm);
  gp.set_signature(co_.AsymSign(gp.data(),
                                "",
                                ss_->PrivateKey(pt),
                                crypto::STRING_STRING));
  std::string ser_gp;
  gp.SerializeToString(&ser_gp);
  return ser_gp;
}

void MessageHandler::CreateSignature(const std::string &buffer_name,
                                     const BufferPacketType &type,
                                     std::string *signed_request,
                                     std::string *signed_public_key) {
  maidsafe::PacketType pt = PacketHandler_PacketType(type);
  *signed_public_key = co_.AsymSign(ss_->PublicKey(pt),
                                    "",
                                    ss_->PrivateKey(pt),
                                    crypto::STRING_STRING);
  std::string non_hex_buffer_name("");
  base::decode_from_hex(buffer_name, &non_hex_buffer_name);
  *signed_request = co_.AsymSign(
      co_.Hash(ss_->PublicKey(pt) + *signed_public_key +
               non_hex_buffer_name, "", crypto::STRING_STRING, false),
      "",
      ss_->PrivateKey(pt),
      crypto::STRING_STRING);
}

void MessageHandler::IterativeStoreMsgs(
    boost::shared_ptr<SendMessagesData> data) {
  if (data->is_calledback) {
    return;
  }
  if (data->stores_done == static_cast<int>(data->receivers.size())) {
    packethandler::StoreMessagesResult result;
    if (data->successful_stores == 0)
      result.set_result(kNack);
    else
      result.set_result(kAck);
    result.set_stored_msgs(data->successful_stores);
    for (int i = 0; i < static_cast<int>(data->no_auth_rec.size()); ++i) {
      result.add_failed(data->no_auth_rec[i]);
    }
    data->is_calledback = true;
    std::string str_result;
    result.SerializeToString(&str_result);
    data->cb(str_result);
    return;
  }

  if (data->index < static_cast<int>(data->receivers.size())) {
    int msgs_to_send = parallelSendMsgs - data->active_sends;

    for (int n = 0;
         n < msgs_to_send &&
         data->index < static_cast<int>(data->receivers.size());
         ++n) {
      ++data->index;
      std::string rec_public_key;
      std::string ser_packet="";
      std::string sys_packet_name;
      switch (data->p_type) {
        case packethandler::ADD_CONTACT_RQST:
        case packethandler::INSTANT_MSG:
            sys_packet_name = co_.Hash(data->receivers[data->index].id,
                                       "",
                                       crypto::STRING_STRING,
                                       true);
            break;
        default : sys_packet_name = data->receivers[data->index].id;
      }
      ++data->active_sends;
      StoreMessage(data->index, data);
    }
  }
}

void MessageHandler::StoreMessage(
    int index,
    boost::shared_ptr<SendMessagesData> data) {
  if (data->is_calledback) {
    return;
  }
  std::string ser_msg = CreateMessage(data->msg,
                                      data->receivers[index].public_key,
                                      data->m_type,
                                      data->p_type,
                                      data->timestamp);
  std::string bufferpacketname = co_.Hash(data->receivers[index].id+"BUFFER",
                                          "",
                                          crypto::STRING_STRING,
                                          true);
#ifdef DEBUG
  // printf("\nBufferpacket name (Saving):\n%s\n\n", bufferpacketname.c_str());
#endif
  std::string signed_public_key, signed_request;
  CreateSignature(bufferpacketname,
                  data->p_type,
                  &signed_request,
                  &signed_public_key);
  sm_->StorePacket(bufferpacketname,
                   ser_msg,
                   signed_request,
                   ss_->PublicKey(PacketHandler_PacketType(data->p_type)),
                   signed_public_key,
                   BUFFER_PACKET_MESSAGE,
                   true,
                   boost::bind(&MessageHandler::StoreMessage_Callback,
                               this,
                               _1,
                               index,
                               data));
}

void MessageHandler::StoreMessage_Callback(
    const std::string &result,
    int index,
    boost::shared_ptr<SendMessagesData> data) {
  if (data->is_calledback) {
    return;
  }
  UpdateResponse result_msg;
  if ((result_msg.ParseFromString(result)) &&
      (result_msg.result() == kAck)) {
    ++data->successful_stores;
  } else {
    data->no_auth_rec.push_back(data->receivers[index].id);
  }
  --data->active_sends;
  ++data->stores_done;
  IterativeStoreMsgs(data);
}

maidsafe::PacketType MessageHandler::PacketHandler_PacketType(
    const BufferPacketType &type) {
  //  MPID_BP, MAID_BP, PMID_BP
  switch (type) {
    case MAID_BP: return maidsafe::MAID;
    case PMID_BP: return maidsafe::PMID;
    default: return maidsafe::MPID;
  }
}

}  //  namespace maidsafe
