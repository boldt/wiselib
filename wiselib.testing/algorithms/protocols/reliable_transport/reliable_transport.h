/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

#ifndef RELIABLE_TRANSPORT_H
#define RELIABLE_TRANSPORT_H

#include <util/delegates/delegate.hpp>
#include <util/base_classes/radio_base.h>

#include "reliable_transport_message.h"
#include <util/pstl/map_static_vector.h>

namespace wiselib {
	
	/**
	 * @brief
	 * 
	 * @ingroup
	 * 
	 * @tparam 
	 */
	template<
		typename OsModel_P,
		typename ChannelId_P,
		typename Radio_P,
		typename Timer_P
	>
	class ReliableTransport : public RadioBase<OsModel_P, typename Radio_P::node_id_t, typename OsModel_P::size_t, typename OsModel_P::block_data_t> {
		
		public:
			//{{{ Typedefs & Enums
			typedef ReliableTransport<OsModel_P, ChannelId_P, Radio_P, Timer_P> self_type;
			
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			typedef typename OsModel::size_t size_t;
			typedef ChannelId_P ChannelId;
			
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			typedef typename Radio::message_id_t message_id_t;
			typedef Timer_P Timer;
			
			typedef ReliableTransportMessage<OsModel, ChannelId, Radio> Message;
			typedef typename Message::sequence_number_t sequence_number_t;
			
			class Endpoint;
			
			typedef delegate2<bool, Message&, Endpoint&> produce_callback_t;
			typedef delegate2<void, Message&, Endpoint&> consume_callback_t;
			typedef delegate2<void, int, Endpoint&> event_callback_t;
			
			enum SpecialNodeIds {
				BROADCAST_ADDRESS = Radio::BROADCAST_ADDRESS,
				NULL_NODE_ID = Radio::NULL_NODE_ID
			};
			
			enum Restrictions {
				MAX_MESSAGE_LENGTH = Radio::MAX_MESSAGE_LENGTH - Message::HEADER_SIZE,
				RESEND_TIMEOUT = 5000, MAX_RESENDS = 3,
				ANSWER_TIMEOUT = 2500,
			};
			
			enum ReturnValues {
				SUCCESS = OsModel::SUCCESS, ERR_UNSPEC = OsModel::ERR_UNSPEC
			};
			
			enum { npos = (size_type)(-1) };
			
			enum Events {
				EVENT_ABORT, EVENT_OPEN, EVENT_CLOSE
			};
			
			//}}}
			
			class Endpoint {
				// {{{
				public:
					void init(node_id_t remote_address, const ChannelId& channel, bool initiator, produce_callback_t p, consume_callback_t c, event_callback_t a) {
						remote_address_ = remote_address;
						produce_ = p;
						consume_ = c;
						event_ = a;
						sending_sequence_number_ = 0;
						receiving_sequence_number_ = 0;
						channel_id_ = channel;
						initiator_ = initiator;
						
						request_open_ = false;
						request_send_ = false;
						request_close_ = false;
						open_ = false;
						expect_answer_ = false;
					}
					
					sequence_number_t sending_sequence_number() { return sending_sequence_number_; }
					void increase_sending_sequence_number() { sending_sequence_number_++; }
					sequence_number_t receiving_sequence_number() { return receiving_sequence_number_; }
					void increase_receiving_sequence_number() { receiving_sequence_number_++; }
					
					const ChannelId& channel() { return channel_id_; }
					
					size_type produce(Message& msg) {
						return produce_(msg, *this);
					}
					
					void consume(Message& msg) {
						consume_(msg, *this);
					}
					
					void abort_produce() {
						event_(EVENT_ABORT, *this);
						//produce_(0, 0, *this);
						//DBG("+++++++++++++ TODO: pass abort_send to application!");
					}
					
					bool used() { return produce_ || consume_; }
					bool wants_something() {
						return wants_send() || wants_open() || wants_close();
					}
					
					void request_send() { request_send_ = true; }
					bool wants_send() { return request_send_; }
					void comply_send() { request_send_ = false; }
					
					void request_open() {
						sending_sequence_number_ = 0;
						receiving_sequence_number_ = 0;
						request_open_ = true;
					}
					bool wants_open() { return request_open_; }
					void open() {
						event_(EVENT_OPEN, *this);
						
						sending_sequence_number_ = 0;
						receiving_sequence_number_ = 0;
						request_open_ = false;
						request_close_ = false;
						open_ = true;
						expect_answer_ = false;
					}
					bool is_open() { return open_; }
					
					void request_close() {
						open_ = false;
						request_close_ = true;
					}
					bool wants_close() { return request_close_; }
					void close() {
						event_(EVENT_CLOSE, *this);
						
						receiving_sequence_number_ = 0;
						sending_sequence_number_ = 0;
						request_open_ = false;
						request_close_ = false;
						open_ = false;
					}
					
					bool initiator() { return initiator_; }
					
					node_id_t remote_address() { return remote_address_; }
					void set_remote_address(node_id_t x) { remote_address_ = x; }
					
					bool expects_answer() { return expect_answer_; }
					void set_expect_answer(bool e) { expect_answer_ = e; }
				
				private:
					node_id_t remote_address_;
					produce_callback_t produce_;
					consume_callback_t consume_;
					event_callback_t event_;
					sequence_number_t sending_sequence_number_;
					sequence_number_t receiving_sequence_number_;
					
					ChannelId channel_id_;
					bool initiator_;
					bool request_open_;
					bool request_send_;
					bool request_close_;
					bool open_;
					bool expect_answer_;
				// }}}
			};
			
			enum { MAX_ENDPOINTS = 8 };
			typedef Endpoint Endpoints[MAX_ENDPOINTS];
		
			int init(typename Radio::self_pointer_t radio, typename Timer::self_pointer_t timer, bool reg_receiver = true) {
				radio_ = radio;
				timer_ = timer;
				sending_channel_idx_ = 0;
				is_sending_ = false;
				
				if(reg_receiver) {
					radio_->template reg_recv_callback<self_type, &self_type::on_receive>(this);
				}
				return SUCCESS;
			}
			
			int id() {
				return radio_->id();
			}
			
			int enable_radio() {
				return radio_->enable_radio();
			}
			
			int disable_radio() {
				return radio_->disable_radio();
			}
			
			int register_endpoint(node_id_t addr, const ChannelId& channel, bool initiator, produce_callback_t produce, consume_callback_t consume, event_callback_t event) {
				size_type idx = find_or_create_endpoint(channel, initiator, true);
				if(idx == npos) {
					return ERR_UNSPEC;
				}
				else {
					endpoints_[idx].init(addr, channel, initiator, produce, consume, event);
					return SUCCESS;
				}
			}
			
			Endpoint& get_endpoint(const ChannelId& channel, bool initiator, bool& found) {
				size_type idx = find_or_create_endpoint(channel, initiator, false);
				if(idx == npos) {
					found = false;
					return endpoints_[0];
				}
				found = true;
				return endpoints_[idx];
			}
			
			int open(const ChannelId& channel, bool initiator = true) {
				bool found;
				Endpoint& ep = get_endpoint(channel, initiator, found);
				if(found) {
					return open(ep);
				}
				return ERR_UNSPEC;
			}
				
			int open(Endpoint& ep) {
				if(!ep.is_open() && !ep.wants_open()) {
					ep.request_open();
					return SUCCESS;
				}
				return ERR_UNSPEC;
			}
			
			int close(const ChannelId& channel, bool initiator) {
				bool found;
				Endpoint& ep = get_endpoint(channel, initiator, found);
				if(found && ep.is_open()) {
					ep.request_close();
				}
			}
			
			void expect_answer(Endpoint& ep) {
				ep.set_expect_answer(true);
				timer_->template set_timer<self_type, &self_type::on_answer_timeout>(ANSWER_TIMEOUT, this, &ep);
			}
			
			node_id_t remote_address(const ChannelId& channel, bool initiator) {
				size_type idx = find_or_create_endpoint(channel, initiator, false);
				if(idx == npos) { return NULL_NODE_ID; }
				return endpoints_[idx].remote_address();
			}
			
			void set_remote_address(const ChannelId& channel, bool initiator, node_id_t addr) {
				size_type idx = find_or_create_endpoint(channel, initiator, false);
				if(idx == npos) { return; }
				endpoints_[idx].set_remote_address(addr);
			}
			
			int destruct(const ChannelId& channel, bool initiator) {
				size_type idx = find_or_create_endpoint(channel, initiator, false);
				if(idx == npos) {
					DBG("destruct: channel not found");
					return ERR_UNSPEC;
				}
				endpoints_[idx].destruct();
				return SUCCESS;
			}
			
			int request_send(const ChannelId& channel, bool initiator) {
				size_type idx = find_or_create_endpoint(channel, initiator, false);
				if(idx == npos) {
					DBG("request_send: channel not found");
					return ERR_UNSPEC;
				}
				else {
					DBG("request_send for idx %d", idx);
					endpoints_[idx].request_send();
					check_send();
					//if(!is_sending_) {
						//switch_sending_endpoint();
						//try_send();
					//}
				}
				return SUCCESS;
			}
			
			void on_receive(node_id_t from, typename Radio::size_t len, block_data_t* data) {
				Message &msg = *reinterpret_cast<Message*>(data);
				if(msg.type() != Message::MESSAGE_TYPE) {
					return;
				}
				
				DBG("transport recv chan %x.%x", msg.channel().rule(), msg.channel().value());
				
				if(msg.is_ack()) {
					on_receive_ack(from, msg);
				}
				else {
					send_ack(from, msg);
					on_receive_data(msg);
				}
			}
			
		
		private:
			Endpoint& sending_endpoint() { return endpoints_[sending_channel_idx_]; }
			
			size_type find_or_create_endpoint(const ChannelId& channel, bool initiator, bool create) {
				size_type free = npos;
				for(size_type i = 0; i < MAX_ENDPOINTS; i++) {
					if(free == npos && !endpoints_[i].used()) {
						free = i;
					}
					else if(endpoints_[i].channel() == channel && endpoints_[i].initiator() == initiator) {
						return i;
					}
				}
				return create ? free : npos;
			}
			
			void on_answer_timeout(void *ep_) {
				Endpoint &ep = *reinterpret_cast<Endpoint*>(ep_);
				if(ep.is_open() && ep.expects_answer()) {
					ep.abort_produce();
					ep.close();
				}
			}
			
			/**
			 * Switch to next channel for sending.
			 */
			bool switch_sending_endpoint() {
				size_type ole = sending_channel_idx_;
				for(sending_channel_idx_++ ; sending_channel_idx_ < MAX_ENDPOINTS; sending_channel_idx_++) {
					//DBG("switch idx %d used %d destruct %d send %d",
							//sending_channel_idx_, sending_endpoint().used(),
							//sending_endpoint().wants_destruct(), sending_endpoint().wants_send());
					
					if(sending_endpoint().used() && sending_endpoint().wants_something()) {
						is_sending_ = true;
						return true;
					}
				}
				for(sending_channel_idx_ = 0; sending_channel_idx_ <= ole; sending_channel_idx_++) {
					//DBG("switch idx %d used %d destruct %d send %d",
							//sending_channel_idx_, sending_endpoint().used(),
							//sending_endpoint().wants_destruct(), sending_endpoint().wants_send());
					
					if(sending_endpoint().used() && sending_endpoint().wants_something()) {
						is_sending_ = true;
						return true;
					}
				}
				is_sending_ = false;
				return false;
			}
			
			///@name Sending.
			///@{
			//{{{
			
			void check_send() {
				DBG("check_send")
				if(is_sending_) {
					DBG("check_send: currently sending idx %d", sending_channel_idx_);
					return;
				}
				if(switch_sending_endpoint()) {
					int flags = (sending_endpoint().initiator() ? Message::FLAG_INITIATOR : 0);
					if(sending_endpoint().wants_open()) { flags |= Message::FLAG_OPEN; }
					if(sending_endpoint().wants_close()) { flags |= Message::FLAG_CLOSE; }
					
					sending_.set_channel(sending_endpoint().channel());
					sending_.set_sequence_number(sending_endpoint().sending_sequence_number());
					sending_.set_flags(flags);
					
					if(sending_endpoint().wants_send()) {
						sending_endpoint().comply_send();
						bool send = sending_endpoint().produce(sending_);
						if(!send) {
							DBG("check_send: idx %d didnt produce anything!", sending_channel_idx_);
							is_sending_ = false;
							check_send();
							return;
						}
					}
					
					try_send();
				}
				else {
					DBG("check_send: no sending endpoint found");
				}
			}
			
			/**
			 * When receiving ack, schedule next send.
			 */
			void on_receive_ack(node_id_t from, Message& msg) {
				if(msg.channel() == sending_endpoint().channel() &&
						msg.initiator() == sending_endpoint().initiator() &&
						msg.sequence_number() == sending_endpoint().sending_sequence_number()) {
					
					if(sending_endpoint().wants_open()) {
						sending_endpoint().open();
					}
					
					if(!sending_endpoint().open()) { return; }
					
					DBG("@%d recv ack seqnr=%d ack_timer=%d chan=%x.%x/%d", radio_->id(), msg.sequence_number(), ack_timer_,
							msg.channel().rule(), msg.channel().value(), msg.initiator());
					ack_timer_++; // invalidate running ack timer
					sending_endpoint().increase_sending_sequence_number();
					
					if(sending_endpoint().wants_close()) {
						sending_endpoint().close();
					}
					
					is_sending_ = false;
					check_send();
				}
				else {
					DBG("@%d ignoring ack from %d. mychan=%d.%d ackchan=%d.%d myseqnr=%d ackseqnr=%d init=%d ackinit=%d",
							radio_->id(), from,
							sending_endpoint().channel().rule(), sending_endpoint().channel().value(),
							msg.channel().rule(), msg.channel().value(),
							sending_endpoint().sending_sequence_number(), msg.sequence_number(),
							sending_endpoint().initiator(), msg.initiator());
					// ignore ack for wrong channel
				}
			}
			
			/**
			 * Try sending the current buffer contents
			 */
			void try_send() {
				if(!is_sending_) {
					DBG("try send: is not sending");
					return;
				}
				
				resends_ = 0;
				if(sending_.size()) {
					try_send(0);
				}
				else {
					is_sending_ = false;
				}
			}
			
			/// ditto.
			void try_send(void *) {
				if(!is_sending_) {
					return;
				}
				
				DBG("@%d try_send ack timer %d seqnr %d chan=%x.%x/%d", radio_->id(), ack_timer_, sending_endpoint().sending_sequence_number(),
							sending_.channel().rule(), sending_.channel().value(), sending_.initiator());
				radio_->send(sending_endpoint().remote_address(), sending_.size(), sending_.data());
				resends_++;
				//ack_timeout_channel_ = sending_endpoint().channel();
				//ack_timeout_sequence_number_ = sending_endpoint().sequence_number();
				timer_->template set_timer<self_type, &self_type::ack_timeout>(RESEND_TIMEOUT, this, (void*)ack_timer_);
			}
			
			void ack_timeout(void *ack_timer) {
				//if(is_sending_ && sending_endpoint().used() && sending_endpoint().channel() == ack_timeout_channel_ &&
						//sending_endpoint().sequence_number() == ack_timeout_sequence_number_) {
					
				if(is_sending_ && ((size_type)ack_timer == ack_timer_)) {
					DBG("ack_timeout @%d resends=%d ack timer %d sqnr %d idx %d chan=%x.%x/%d", radio_->id(), resends_, ack_timer_, sending_endpoint().sending_sequence_number(), sending_channel_idx_,
							sending_.channel().rule(), sending_.channel().value(), sending_.initiator());
					if(resends_ >= MAX_RESENDS) {
						sending_endpoint().abort_produce();
						sending_endpoint().close();
						is_sending_ = false;
						check_send();
					}
					else {
						try_send(0);
					}
				}
			}
			
			//}}}
			///@}
			
			///@name Receiving.
			///@{
			//{{{
			
			void on_receive_data(Message& msg) {
				size_type idx = find_or_create_endpoint(msg.channel(), !msg.initiator(), false);
				if(idx == npos) {
					DBG("on_receive_data: ignoring message of unkonwn channel %x.%x", msg.channel().rule(), msg.channel().value());
					return;
				}
				
				if(msg.is_open()) {
					DBG("node %d // opening receiver", radio_->id());
					endpoints_[idx].open();
				}
				
				if(!msg.is_open() && msg.sequence_number() != endpoints_[idx].receiving_sequence_number()) {
					DBG("on_receive_data: ignoring message of wrong seqnr %d (expected: %d)", msg.sequence_number(), endpoints_[idx].receiving_sequence_number());
					return;
				}
					
					//if(msg.is_open()) {
						//if(endpoints_[idx].is_open()) { return; }
						//else { endpoints_[idx].open(); }
					//}
					
					if(msg.is_close()) {
						if(endpoints_[idx].is_open()) { endpoints_[idx].close(); }
						else { return; }
					}
					
					
					DBG("on_receive_data: om nom nom");
					if(msg.payload_size()) {
						endpoints_[idx].set_expect_answer(false);
						endpoints_[idx].consume(msg);
					}
					
					endpoints_[idx].increase_receiving_sequence_number();
					
			}
			
			void send_ack(node_id_t to, Message& msg) {
				msg.set_payload(0, 0);
				
				//msg.set_subtype(Message::SUBTYPE_ACK);
				msg.set_flags(msg.flags() | Message::FLAG_ACK);
				//msg.set_channel(msg.channel());
				radio_->send(to, msg.size(), msg.data());
			}
			
			//}}}
			///@}
			
			typename Radio::self_pointer_t radio_;
			typename Timer::self_pointer_t timer_;
			
			Endpoints endpoints_;
			Message sending_;
			
			ChannelId ack_timeout_channel_;
			size_type sending_channel_idx_;
			size_type ack_timer_;
			size_type resends_;
			//sequence_number_t ack_timeout_sequence_number_;
			bool is_sending_;
		
	}; // ReliableTransport
}

#endif // RELIABLE_TRANSPORT_H

