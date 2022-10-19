#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"

static int32_t is_pow_of_2(uint32_t size)
{
	return !(size&(size-1));
}

static uint8_t GetK(uint32_t size)
{
	uint8_t k = 0;
	if(!is_pow_of_2(size))
	{
		size = (size << 1);
	}
	//�����Ϊ1λ����,����λȫ����0
	while(size > 1)
	{
		k++;
		size = size >> 1;
	}
	return k;
}



wpacket_t wpacket_create(uint32_t size)
{
	uint8_t k = GetK(size);
	wpacket_t w;
	size = 1 << k;	
	w = reinterpret_cast<wpacket_t>(calloc(1,sizeof(*w)));
	w->factor = k;
	w->wpos = sizeof(w->len);
	w->buf = buffer_create_and_acquire(0,size);
	w->writebuf = buffer_acquire(0,w->buf);
	w->len = (uint32_t*)w->buf->buf;
	*(w->len) = 0;
	w->buf->size = sizeof(w->len);
	w->begin_pos = 0;
	w->data_size = sizeof(*(w->len));
	return w;
}

wpacket_t wpacket_create_by_rpacket(struct rpacket *r)
{
	wpacket_t w = reinterpret_cast<wpacket_t>(calloc(1,sizeof(*w)));
	w->factor = 0;
	w->writebuf = 0;
	w->begin_pos = r->begin_pos;
	w->buf = buffer_acquire(0,r->buf);
	w->len = 0;//��������֮ǰlenû������
	w->wpos = 0;
	w->data_size = r->len + sizeof(r->len);
	return w;
}

write_pos wpacket_get_writepos(wpacket_t w)
{
	write_pos wp = {w->writebuf,w->wpos};
	return wp;
}

void wpacket_destroy(wpacket_t *w)
{
	buffer_release(&(*w)->buf);
	buffer_release(&(*w)->writebuf);
	free(*w);
	*w = 0;
}

static void wpacket_expand(wpacket_t w)
{
	uint32_t size;
	w->factor <<= 1;
	size = 1 << w->factor;
	w->writebuf->next = buffer_create_and_acquire(0,size);
    w->writebuf = buffer_acquire(w->writebuf,w->writebuf->next); 
	w->wpos = 0;
}


static void wpacket_copy(wpacket_t w,buffer_t buf)
{
	char *ptr = buf->buf;
	buffer_t tmp_buf = w->buf;
	uint32_t copy_size;
	while(tmp_buf)
	{
		copy_size = tmp_buf->size - w->wpos;
		memcpy(ptr,tmp_buf->buf,copy_size);
		ptr += copy_size;
		w->wpos = 0;
		tmp_buf = tmp_buf->next;
	}
}

static void wpacket_write(wpacket_t w,char *addr,uint32_t size)
{
	char *ptr = addr;
	uint32_t copy_size;
	buffer_t tmp;
	uint8_t k;
	if(!w->writebuf)
	{
		/*wpacket����rpacket����ģ�����ִ��дʱ������
		* ִ�����wpacket�͹���ʱ�����rpacket���ٹ���buffer
		*/
		k = GetK(*w->len);
		w->factor = k;
		tmp = buffer_create_and_acquire(0,1 << k);
		wpacket_copy(w,tmp);
		w->begin_pos = 0;
		w->len = (uint32_t*)tmp->buf;
		w->wpos = sizeof(*w->len);
		w->buf = buffer_acquire(w->buf,tmp);
		w->writebuf = buffer_acquire(w->writebuf,w->buf);
	}
	while(size)
	{
		copy_size = w->buf->capacity - w->wpos;
		if(copy_size == 0)
		{
			wpacket_expand(w);//�ռ䲻��,��չ
			copy_size = w->buf->capacity - w->wpos;
		}
		copy_size = copy_size > size ? size:copy_size;
		memcpy(w->writebuf->buf + w->wpos,ptr,copy_size);
		w->writebuf->size += copy_size;
		(*w->len) += copy_size;
		w->wpos += copy_size;
		ptr += copy_size;
		size -= copy_size;
		w->data_size += copy_size;
	}
}


void wpacket_write_uint8(wpacket_t w,uint8_t value)
{
	wpacket_write(w,(char*)&value,sizeof(value));
}

void wpacket_write_uint16(wpacket_t w,uint16_t value)
{
	wpacket_write(w,(char*)&value,sizeof(value));
}

void wpacket_write_uint32(wpacket_t w,uint32_t value)
{
	wpacket_write(w,(char*)&value,sizeof(value));
}

void wpacket_write_uint64(wpacket_t w,uint64_t value)
{
	wpacket_write(w,(char*)&value,sizeof(value));
}

void wpacket_write_double(wpacket_t w ,double value)
{
	wpacket_write(w,(char*)&value,sizeof(value));
}

static void wpacket_rewrite(write_pos *wp,char *addr,uint32_t size)
{
	char *ptr = addr;
	uint32_t copy_size;
	uint32_t pos = wp->wpos;
	while(size)
	{
		copy_size = wp->buf->capacity - pos;
		copy_size = copy_size > size ? size:copy_size;
		memcpy(wp->buf->buf + pos,ptr,copy_size);
		ptr += copy_size;
		size -= copy_size;
		pos += copy_size;
		if(size && pos >= wp->buf->capacity)
		{
			assert(wp->buf->next);
			wp->buf = wp->buf->next;
			pos = 0;
		}

	}
}

void wpacket_rewrite_uint8(write_pos *wp,uint8_t value)
{
	wpacket_rewrite(wp,(char*)&value,sizeof(value));
}

void wpacket_rewrite_uint16(write_pos *wp,uint16_t value)
{
	wpacket_rewrite(wp,(char*)&value,sizeof(value));
}

void wpacket_rewrite_uint32(write_pos *wp,uint32_t value)
{
	wpacket_rewrite(wp,(char*)&value,sizeof(value));
}

void wpacket_rewrite_uint64(write_pos *wp,uint64_t value)
{
	wpacket_rewrite(wp,(char*)&value,sizeof(value));
}

void wpacket_rewrite_double(write_pos *wp,double value)
{
	wpacket_rewrite(wp,(char*)&value,sizeof(value));
}

void wpacket_write_string(wpacket_t w ,const char *value)
{
	wpacket_write_binary(w,value,strlen(value)+1);
}

void wpacket_write_binary(wpacket_t w,const void *value,uint32_t size)
{
	assert(value);
	wpacket_write_uint32(w,size);
	wpacket_write(w,(char*)value,size);
}
