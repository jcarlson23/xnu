/*
 * Copyright (c) 1999-2007 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <stdlib.h>
#include <mach/mach.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/vm_statistics.h>

#define MACH_MSG_TRAP(msg, opt, ssize, rsize, rname, to, not) \
	 mach_msg_trap((msg), (opt), (ssize), (rsize), (rname), (to), (not))

#define LIBMACH_OPTIONS	(MACH_SEND_INTERRUPT|MACH_RCV_INTERRUPT)

/*
 *	Routine:	mach_msg
 *	Purpose:
 *		Send and/or receive a message.  If the message operation
 *		is interrupted, and the user did not request an indication
 *		of that fact, then restart the appropriate parts of the
 * 		operation.
 */
mach_msg_return_t
mach_msg(msg, option, send_size, rcv_size, rcv_name, timeout, notify)
	mach_msg_header_t *msg;
	mach_msg_option_t option;
	mach_msg_size_t send_size;
	mach_msg_size_t rcv_size;
	mach_port_t rcv_name;
	mach_msg_timeout_t timeout;
	mach_port_t notify;
{
	mach_msg_return_t mr;

	/*
	 * Consider the following cases:
	 *	1) Errors in pseudo-receive (eg, MACH_SEND_INTERRUPTED
	 *	plus special bits).
	 *	2) Use of MACH_SEND_INTERRUPT/MACH_RCV_INTERRUPT options.
	 *	3) RPC calls with interruptions in one/both halves.
	 *
	 * We refrain from passing the option bits that we implement
	 * to the kernel.  This prevents their presence from inhibiting
	 * the kernel's fast paths (when it checks the option value).
	 */

	mr = MACH_MSG_TRAP(msg, option &~ LIBMACH_OPTIONS,
			   send_size, rcv_size, rcv_name,
			   timeout, notify);
	if (mr == MACH_MSG_SUCCESS)
		return MACH_MSG_SUCCESS;

	if ((option & MACH_SEND_INTERRUPT) == 0)
		while (mr == MACH_SEND_INTERRUPTED)
			mr = MACH_MSG_TRAP(msg,
				option &~ LIBMACH_OPTIONS,
				send_size, rcv_size, rcv_name,
				timeout, notify);

	if ((option & MACH_RCV_INTERRUPT) == 0)
		while (mr == MACH_RCV_INTERRUPTED)
			mr = MACH_MSG_TRAP(msg,
				option &~ (LIBMACH_OPTIONS|MACH_SEND_MSG),
				0, rcv_size, rcv_name,
				timeout, notify);

	return mr;
}

/*
 *	Routine:	mach_msg_overwrite
 *	Purpose:
 *		Send and/or receive a message.  If the message operation
 *		is interrupted, and the user did not request an indication
 *		of that fact, then restart the appropriate parts of the
 * 		operation.
 *
 *		Distinct send and receive buffers may be specified.  If
 *		no separate receive buffer is specified, the msg parameter
 *		will be used for both send and receive operations.
 *
 *		In addition to a distinct receive buffer, that buffer may
 *		already contain scatter control information to direct the
 *		receiving of the message.
 */
mach_msg_return_t
mach_msg_overwrite(msg, option, send_size, rcv_limit, rcv_name, timeout, 
		   notify, rcv_msg, rcv_scatter_size)
	mach_msg_header_t *msg;
	mach_msg_option_t option;
	mach_msg_size_t send_size;
	mach_msg_size_t rcv_limit;
	mach_port_t rcv_name;
	mach_msg_timeout_t timeout;
	mach_port_t notify;
	mach_msg_header_t *rcv_msg;
	mach_msg_size_t rcv_scatter_size;
{
	mach_msg_return_t mr;

	/*
	 * Consider the following cases:
	 *	1) Errors in pseudo-receive (eg, MACH_SEND_INTERRUPTED
	 *	plus special bits).
	 *	2) Use of MACH_SEND_INTERRUPT/MACH_RCV_INTERRUPT options.
	 *	3) RPC calls with interruptions in one/both halves.
	 *
	 * We refrain from passing the option bits that we implement
	 * to the kernel.  This prevents their presence from inhibiting
	 * the kernel's fast paths (when it checks the option value).
	 */

	mr = mach_msg_overwrite_trap(msg, option &~ LIBMACH_OPTIONS,
			   send_size, rcv_limit, rcv_name,
			   timeout, notify, rcv_msg, rcv_scatter_size);
	if (mr == MACH_MSG_SUCCESS)
		return MACH_MSG_SUCCESS;

	if ((option & MACH_SEND_INTERRUPT) == 0)
		while (mr == MACH_SEND_INTERRUPTED)
			mr = mach_msg_overwrite_trap(msg,
				option &~ LIBMACH_OPTIONS,
				send_size, rcv_limit, rcv_name,
				timeout, notify, rcv_msg, rcv_scatter_size);

	if ((option & MACH_RCV_INTERRUPT) == 0)
		while (mr == MACH_RCV_INTERRUPTED)
			mr = mach_msg_overwrite_trap(msg,
				option &~ (LIBMACH_OPTIONS|MACH_SEND_MSG),
				0, rcv_limit, rcv_name,
				timeout, notify, rcv_msg, rcv_scatter_size);

	return mr;
}


mach_msg_return_t
mach_msg_send(mach_msg_header_t *msg)
{
	return mach_msg(msg, MACH_SEND_MSG,
			msg->msgh_size, 0, MACH_PORT_NULL,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}

mach_msg_return_t
mach_msg_receive(mach_msg_header_t *msg)
{
	return mach_msg(msg, MACH_RCV_MSG,
			0, msg->msgh_size, msg->msgh_local_port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}


static void
mach_msg_destroy_port(mach_port_t port, mach_msg_type_name_t type)
{
    if (MACH_PORT_VALID(port)) switch (type) {
      case MACH_MSG_TYPE_MOVE_SEND:
      case MACH_MSG_TYPE_MOVE_SEND_ONCE:
	/* destroy the send/send-once right */
	(void) mach_port_deallocate(mach_task_self_, port);
	break;

      case MACH_MSG_TYPE_MOVE_RECEIVE:
	/* destroy the receive right */
	(void) mach_port_mod_refs(mach_task_self_, port,
				  MACH_PORT_RIGHT_RECEIVE, -1);
	break;

      case MACH_MSG_TYPE_MAKE_SEND:
	/* create a send right and then destroy it */
	(void) mach_port_insert_right(mach_task_self_, port,
				      port, MACH_MSG_TYPE_MAKE_SEND);
	(void) mach_port_deallocate(mach_task_self_, port);
	break;

      case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	/* create a send-once right and then destroy it */
	(void) mach_port_extract_right(mach_task_self_, port,
				       MACH_MSG_TYPE_MAKE_SEND_ONCE,
				       &port, &type);
	(void) mach_port_deallocate(mach_task_self_, port);
	break;
    }
}

static void
mach_msg_destroy_memory(vm_offset_t addr, vm_size_t size)
{
    if (size != 0)
	(void) vm_deallocate(mach_task_self_, addr, size);
}


/*
 *	Routine:	mach_msg_destroy
 *	Purpose:
 *		mach_msg_destroy is useful in two contexts.
 *
 *		First, it can deallocate all port rights and
 *		out-of-line memory in a received message.
 *		When a server receives a request it doesn't want,
 *		it needs this functionality.
 *
 *		Second, it can mimic the side-effects of a msg-send
 *		operation.  The effect is as if the message were sent
 *		and then destroyed inside the kernel.  When a server
 *		can't send a reply (because the client died),
 *		it needs this functionality.
 */
void
mach_msg_destroy(mach_msg_header_t *msg)
{
    mach_msg_bits_t mbits = msg->msgh_bits;

    /*
     *	The msgh_local_port field doesn't hold a port right.
     *	The receive operation consumes the destination port right.
     */

    mach_msg_destroy_port(msg->msgh_remote_port, MACH_MSGH_BITS_REMOTE(mbits));

    if (mbits & MACH_MSGH_BITS_COMPLEX) {
	mach_msg_base_t		*base;
	mach_msg_type_number_t	count, i;
	mach_msg_descriptor_t	*daddr;
	
    	base = (mach_msg_base_t *) msg;
	count = base->body.msgh_descriptor_count;

    	daddr = (mach_msg_descriptor_t *) (base + 1);
	for (i = 0; i < count; i++) {

	    switch (daddr->type.type) {
	    
	    case MACH_MSG_PORT_DESCRIPTOR: {
		    mach_msg_port_descriptor_t *dsc;

		    /* 
		     * Destroy port rights carried in the message 
		     */
		    dsc = &daddr->port;
		    mach_msg_destroy_port(dsc->name, dsc->disposition);
		    daddr = (mach_msg_descriptor_t *)(dsc + 1);
		    break;
	    }

	    case MACH_MSG_OOL_DESCRIPTOR: {
		    mach_msg_ool_descriptor_t *dsc;

		    /* 
		     * Destroy memory carried in the message 
		     */
		    dsc = &daddr->out_of_line;
		    if (dsc->deallocate) {
		        mach_msg_destroy_memory((vm_offset_t)dsc->address,
						dsc->size);
		    }
		    daddr = (mach_msg_descriptor_t *)(dsc + 1);
		    break;
	    }

	    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR: {
		    mach_msg_ool_descriptor_t *dsc;

		    /*
		     * Just skip it.
		     */
		    dsc = &daddr->out_of_line;
		    daddr = (mach_msg_descriptor_t *)(dsc + 1);
		    break;
	    }

	    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
		    mach_port_t             		*ports;
		    mach_msg_ool_ports_descriptor_t	*dsc;
		    mach_msg_type_number_t   		j;

		    /*
		     * Destroy port rights carried in the message 
		     */
		    dsc = &daddr->ool_ports;
		    ports = (mach_port_t *) dsc->address;
		    for (j = 0; j < dsc->count; j++, ports++)  {
		        mach_msg_destroy_port(*ports, dsc->disposition);
		    }

		    /* 
		     * Destroy memory carried in the message 
		     */
		    if (dsc->deallocate) {
		        mach_msg_destroy_memory((vm_offset_t)dsc->address, 
					dsc->count * sizeof(mach_port_t));
		    }
		    daddr = (mach_msg_descriptor_t *)(dsc + 1);
		    break;
	    }
	    }
	}
    }
}

/*
 *	Routine:	mach_msg_server_once
 *	Purpose:
 *		A simple generic server function.  It allows more flexibility
 *		than mach_msg_server by processing only one message request
 *		and then returning to the user.  Note that more in the way
 * 		of error codes are returned to the user; specifically, any
 * 		failing error from mach_msg calls will be returned
 *		(though errors from the demux routine or the routine it
 *		calls will not be).
 */
mach_msg_return_t
mach_msg_server_once(
	boolean_t (*demux)(mach_msg_header_t *, mach_msg_header_t *),
	mach_msg_size_t max_size,
	mach_port_t rcv_name,
	mach_msg_options_t options)
{
	mig_reply_error_t *bufRequest, *bufReply;
	mach_msg_size_t request_size;
	mach_msg_size_t request_alloc;
	mach_msg_size_t trailer_alloc;
	mach_msg_size_t reply_alloc;
	mach_msg_return_t mr;
	kern_return_t kr;
	mach_port_t self = mach_task_self_;

	options &= ~(MACH_SEND_MSG|MACH_RCV_MSG);

	trailer_alloc = REQUESTED_TRAILER_SIZE(options);
	request_alloc = round_page(max_size + trailer_alloc);

	request_size = (options & MACH_RCV_LARGE) ?
    		   request_alloc : max_size + trailer_alloc;

	reply_alloc = round_page((options & MACH_SEND_TRAILER) ? 
			     (max_size + MAX_TRAILER_SIZE) :
			     max_size);

	kr = vm_allocate(self,
		     (vm_address_t *)&bufReply,
		     reply_alloc,
		     VM_MAKE_TAG(VM_MEMORY_MACH_MSG)|TRUE);
	if (kr != KERN_SUCCESS)
		return kr;    

	for (;;) {
		mach_msg_size_t new_request_alloc;

		kr = vm_allocate(self,
			 (vm_address_t *)&bufRequest,
			 request_alloc,
			 VM_MAKE_TAG(VM_MEMORY_MACH_MSG)|TRUE);
		if (kr != KERN_SUCCESS) {
			vm_deallocate(self,
					(vm_address_t)bufReply,
			  		reply_alloc);
			return kr;
		}    
	
		mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG|options,
					  0, request_size, rcv_name,
					  MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	
		if (!((mr == MACH_RCV_TOO_LARGE) && (options & MACH_RCV_LARGE)))
			break;

		new_request_alloc = round_page(bufRequest->Head.msgh_size +
									   trailer_alloc);
		vm_deallocate(self,
				(vm_address_t) bufRequest,
				request_alloc);
		request_size = request_alloc = new_request_alloc;
	}

	if (mr == MACH_MSG_SUCCESS) {
	/* we have a request message */

		(void) (*demux)(&bufRequest->Head, &bufReply->Head);

		if (!(bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
			if (bufReply->RetCode == MIG_NO_REPLY)
				bufReply->Head.msgh_remote_port = MACH_PORT_NULL;
			else if ((bufReply->RetCode != KERN_SUCCESS) &&
					 (bufRequest->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
				/* destroy the request - but not the reply port */
				bufRequest->Head.msgh_remote_port = MACH_PORT_NULL;
				mach_msg_destroy(&bufRequest->Head);
			}
		}

		/*
		 *	We don't want to block indefinitely because the client
		 *	isn't receiving messages from the reply port.
		 *	If we have a send-once right for the reply port, then
		 *	this isn't a concern because the send won't block.
		 *	If we have a send right, we need to use MACH_SEND_TIMEOUT.
		 *	To avoid falling off the kernel's fast RPC path unnecessarily,
		 *	we only supply MACH_SEND_TIMEOUT when absolutely necessary.
		 */
		if (bufReply->Head.msgh_remote_port != MACH_PORT_NULL) {

			mr = mach_msg(&bufReply->Head,
				(MACH_MSGH_BITS_REMOTE(bufReply->Head.msgh_bits) ==
				 MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
				MACH_SEND_MSG|options :
				MACH_SEND_MSG|MACH_SEND_TIMEOUT|options,
				bufReply->Head.msgh_size, 0, MACH_PORT_NULL,
				MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		
			if ((mr != MACH_SEND_INVALID_DEST) &&
				(mr != MACH_SEND_TIMED_OUT))
				goto done_once;
			mr = MACH_MSG_SUCCESS;
		}
		if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
			mach_msg_destroy(&bufReply->Head);
	}

 done_once:
	(void)vm_deallocate(self,
			(vm_address_t) bufRequest,
			request_alloc);
	(void)vm_deallocate(self,
			(vm_address_t) bufReply,
			reply_alloc);
	return mr;
}

/*
 *	Routine:	mach_msg_server
 *	Purpose:
 *		A simple generic server function.  Note that changes here
 * 		should be considered for duplication above.
 */
mach_msg_return_t
mach_msg_server(
	boolean_t (*demux)(mach_msg_header_t *, mach_msg_header_t *),
	mach_msg_size_t max_size,
	mach_port_t rcv_name,
	mach_msg_options_t options)
{
	mig_reply_error_t *bufRequest, *bufReply;
	mach_msg_size_t request_size;
	mach_msg_size_t new_request_alloc;
	mach_msg_size_t request_alloc;
	mach_msg_size_t trailer_alloc;
	mach_msg_size_t reply_alloc;
	mach_msg_return_t mr;
	kern_return_t kr;
	mach_port_t self = mach_task_self_;

	options &= ~(MACH_SEND_MSG|MACH_RCV_MSG|MACH_RCV_OVERWRITE);

	reply_alloc = round_page((options & MACH_SEND_TRAILER) ? 
			     (max_size + MAX_TRAILER_SIZE) : max_size);

	kr = vm_allocate(self,
		     (vm_address_t *)&bufReply,
		     reply_alloc,
		     VM_MAKE_TAG(VM_MEMORY_MACH_MSG)|TRUE);
	if (kr != KERN_SUCCESS) 
		return kr;

	request_alloc = 0;
	trailer_alloc = REQUESTED_TRAILER_SIZE(options);
	new_request_alloc = round_page(max_size + trailer_alloc);

	request_size = (options & MACH_RCV_LARGE) ?
    		   new_request_alloc : max_size + trailer_alloc;

	for (;;) {
		if (request_alloc < new_request_alloc) {
			request_alloc = new_request_alloc;
			kr = vm_allocate(self,
			     	(vm_address_t *)&bufRequest,
			     	request_alloc,
			     	VM_MAKE_TAG(VM_MEMORY_MACH_MSG)|TRUE);
			if (kr != KERN_SUCCESS) {
				vm_deallocate(self,
						(vm_address_t)bufReply,
						reply_alloc);
				return kr;
			}
		}
		
		mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG|options,
				0, request_size, rcv_name,
				MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	
		while (mr == MACH_MSG_SUCCESS) {
			/* we have another request message */

			(void) (*demux)(&bufRequest->Head, &bufReply->Head);

			if (!(bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
				if (bufReply->RetCode == MIG_NO_REPLY)
					bufReply->Head.msgh_remote_port = MACH_PORT_NULL;
				else if ((bufReply->RetCode != KERN_SUCCESS) &&
					(bufRequest->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
					/* destroy the request - but not the reply port */
					bufRequest->Head.msgh_remote_port = MACH_PORT_NULL;
					mach_msg_destroy(&bufRequest->Head);
				}
			}

			/*
			 * We don't want to block indefinitely because the client
			 * isn't receiving messages from the reply port.
			 * If we have a send-once right for the reply port, then
			 * this isn't a concern because the send won't block.
			 * If we have a send right, we need to use MACH_SEND_TIMEOUT.
			 * To avoid falling off the kernel's fast RPC path,
			 * we only supply MACH_SEND_TIMEOUT when absolutely necessary.
			 */
			if (bufReply->Head.msgh_remote_port != MACH_PORT_NULL) {
				if (request_alloc == reply_alloc) {
					mig_reply_error_t *bufTemp;

					mr = mach_msg(
						&bufReply->Head,
						(MACH_MSGH_BITS_REMOTE(bufReply->Head.msgh_bits) ==
						 MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
						 MACH_SEND_MSG|MACH_RCV_MSG|options :
						 MACH_SEND_MSG|MACH_RCV_MSG|MACH_SEND_TIMEOUT|options,
						bufReply->Head.msgh_size, request_size, rcv_name,
						MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

					/* swap request and reply */
					bufTemp = bufRequest;
					bufRequest = bufReply;
					bufReply = bufTemp;

				} else {
					mr = mach_msg_overwrite(
						&bufReply->Head,
						(MACH_MSGH_BITS_REMOTE(bufReply->Head.msgh_bits) ==
						 MACH_MSG_TYPE_MOVE_SEND_ONCE) ?
						 MACH_SEND_MSG|MACH_RCV_MSG|options :
						 MACH_SEND_MSG|MACH_RCV_MSG|MACH_SEND_TIMEOUT|options,
						bufReply->Head.msgh_size, request_size, rcv_name,
						MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
						&bufRequest->Head, 0);
				}
				
				if ((mr != MACH_SEND_INVALID_DEST) &&
					(mr != MACH_SEND_TIMED_OUT))
					continue;
			}
			if (bufReply->Head.msgh_bits & MACH_MSGH_BITS_COMPLEX)
				mach_msg_destroy(&bufReply->Head);

			mr = mach_msg(&bufRequest->Head, MACH_RCV_MSG|options,
						  0, request_size, rcv_name,
						  MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

		} /* while (mr == MACH_MSG_SUCCESS) */
		
		if ((mr == MACH_RCV_TOO_LARGE) && (options & MACH_RCV_LARGE)) {
			new_request_alloc = round_page(bufRequest->Head.msgh_size +
								trailer_alloc);
			request_size = new_request_alloc;
			vm_deallocate(self,
						  (vm_address_t) bufRequest,
						  request_alloc);
			continue;
		}

		break;

    } /* for(;;) */

    (void)vm_deallocate(self,
			(vm_address_t) bufRequest,
			request_alloc);
    (void)vm_deallocate(self,
			(vm_address_t) bufReply,
			reply_alloc);
    return mr;
}
