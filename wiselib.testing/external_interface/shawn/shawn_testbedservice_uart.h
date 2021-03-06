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
#ifndef CONNECTOR_SHAWN_TESTBEDSERVICE_UART_H
#define CONNECTOR_SHAWN_TESTBEDSERVICE_UART_H

#include "util/base_classes/uart_base.h"
#include "external_interface/shawn/shawn_types.h"
#include <cstdlib>

namespace wiselib
{
   /** \brief Shawn Implementation of \ref uart_concept "UART Concept"
    *  \ingroup uart_concept
    *  \ingroup shawn_facets
    *
    * Shawn implementation of the \ref uart_concept "UART Concept" ...
    */
   template<typename OsModel_P>
   class ShawnTestbedserviceUartModel
      : public UartBase<OsModel_P, long, unsigned char>
   {
   public:
      typedef OsModel_P OsModel;

      typedef ShawnTestbedserviceUartModel<OsModel> self_type;
      typedef self_type* self_pointer_t;

      typedef unsigned char block_data_t;
      typedef long size_t;
      // --------------------------------------------------------------------
      enum ErrorCodes
      {
         SUCCESS = OsModel::SUCCESS,
         ERR_UNSPEC = OsModel::ERR_UNSPEC
      };
      // -----------------------------------------------------------------------
      ShawnTestbedserviceUartModel( ShawnOs& os )
         : os_      ( os ),
            enabled_( false )
      {
         os_.proc->template reg_uart_recv_callback<self_type, &self_type::receive>( this );
      }
      // -----------------------------------------------------------------------
      int enable_serial_comm()
      {
          enabled_ = true;
          return SUCCESS;
      }
      // -----------------------------------------------------------------------
      int disable_serial_comm()
      {
          enabled_ = false;
          return SUCCESS;
      }
      // -----------------------------------------------------------------------
      int write( size_t len, block_data_t *buf )
      {
         os().proc->send_message( len, buf );
         return SUCCESS;
      }
      // -----------------------------------------------------------------------
      void receive(size_t len, block_data_t *buf)
      {
          self_type::notify_receivers( len, buf );
      }

   private:
      ShawnOs& os()
      { return os_; }
      // --------------------------------------------------------------------
      ShawnOs& os_;
      bool enabled_;
   };
}

#endif
