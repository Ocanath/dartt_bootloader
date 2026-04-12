#include "dartt_flasher.h"
#include "callbacks.h"

DarttFlasher::DarttFlasher(unsigned char addr)
{
	tx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	rx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	ds.address = addr;
	ds.base_offset = 0;
	ds.msg_type = TYPE_SERIAL_MESSAGE;
	ds.tx_buf.buf = tx_buf_mem;
	ds.tx_buf.size = _DARTT_SERIAL_BUFFER_SIZE - _DARTT_NUM_BYTES_COBS_OVERHEAD;		//DO NOT CHANGE. This is for a good reason. See above note
	ds.tx_buf.len = 0;
	ds.rx_buf.buf = rx_buf_mem;
	ds.rx_buf.size = _DARTT_SERIAL_BUFFER_SIZE - _DARTT_NUM_BYTES_COBS_OVERHEAD;	//DO NOT CHANGE. This is for a good reason. See above note
	ds.rx_buf.len = 0;
	ds.blocking_tx_callback = &_tx_blocking_callback;
	ds.user_context_tx = (void*)(&ser);
	ds.blocking_rx_callback = &_rx_blocking_callback;
	ds.user_context_rx = (void*)(&ser);
	ds.timeout_ms = 10;

}

DarttFlasher::~DarttFlasher()
{
	delete[] tx_buf_mem;
	delete[] rx_buf_mem;
}

