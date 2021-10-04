#define MS_CLASS "RTC::AudioPCMObserver"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/AudioPCMObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath> // std::lround()
#include <map>

// #include <stdio.h>
// #define RECORF_FILE_PATH "/Users/renjinkui/Downloads/record.pcm"
// FILE *recordFile;


/*
 * out is null-terminated encode string.
 * return values is out length, exclusive terminating `\0'
 */
unsigned int
base64_encode(const unsigned char *in, unsigned int inlen, char *out);

/*
 * return values is out length
 */
unsigned int
base64_decode(const char *in, unsigned int inlen, unsigned char *out);

//sample rate we want
#define DECODE_PCM_SAMPLERATE 16000
//sampe channel count we want
#define DECODE_PCM_CHANNEL_COUNT 1

//in our system configuration, each audio frame dutration is 10ms 
#define DURATION_MS_PER_FRAME 20

//frame size means sample count for each audio frame
//it's : samplerate * channelCount * DURATION_MS_PER_FRAME / 1000
#define DECODEED_FRAME_SIZE (DECODE_PCM_SAMPLERATE * DECODE_PCM_CHANNEL_COUNT * DURATION_MS_PER_FRAME / 1000)

namespace RTC
{
	/* Instance methods. */

	AudioPCMObserver::AudioPCMObserver(const std::string& id, json& data) : RTC::RtpObserver(id), opusDecoder(NULL)
	{
		MS_TRACE();
        OpenOpusDecoder();
    }

	AudioPCMObserver::~AudioPCMObserver()
	{
		MS_TRACE();
        CloseOpusDecoder();
	}

	void AudioPCMObserver::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		if (producer->GetKind() != RTC::Media::Kind::AUDIO)
			MS_THROW_TYPE_ERROR("not an audio Producer");
        
        MS_DEBUG_TAG(info, "add producer with id: %s, kind: %hhu", producer->producer_id_.c_str(), producer->GetKind());
	}

	void AudioPCMObserver::RemoveProducer(RTC::Producer* producer)
	{
		MS_TRACE();

        MS_DEBUG_TAG(info, "remove producer with id: %s, kind: %hhu", producer->producer_id_.c_str(), producer->GetKind());
	}

	void AudioPCMObserver::ReceiveRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (IsPaused())
			return;

       //we will decode audio rtp payload to PCM

	   uint8_t *payload = packet->GetPayload();
	   size_t payloadLen = packet->GetPayloadLength();
	   
       //each PCM sample is a 16bit little endine integer
       short pcm[DECODEED_FRAME_SIZE] = {0}; 
       //decode payload to PCM now
	   int decodeRet = opus_decode(opusDecoder, payload, payloadLen, pcm, DECODEED_FRAME_SIZE, 0);
       
	  //MS_DEBUG_TAG(info, "audio PCM observer recv RTP, producer:%s size: %u, payload type: %d ,payload length: %u, decodeRet: %d", producer->id.c_str(), packet->GetSize(), packet->GetPayloadType(), packet->GetPayloadLength(), decodeRet);
	   
       //post this pcm result to node js layer by event
       json data = json::object();

	    data["producerId"] = producer->producer_id_;

        char base64[sizeof(short) * DECODEED_FRAME_SIZE + 256] = {0};
        //encode binary pcm to base64 string before post
        int b64EncodeRet = base64_encode((unsigned char *)pcm, DECODEED_FRAME_SIZE * sizeof(short), base64);

        data["pcm"] = std::string(base64);

		Channel::Notifier::Emit(this->id, "pcm", data);
	}


	void AudioPCMObserver::ProducerPaused(RTC::Producer* producer)
	{
		// Remove from the map.
	}

	void AudioPCMObserver::ProducerResumed(RTC::Producer* producer)
	{
		// Insert into the map.
	}

    void AudioPCMObserver::Paused()
	{
		MS_TRACE();
	}

	void AudioPCMObserver::Resumed()
	{
		MS_TRACE();
	}

    void AudioPCMObserver::OpenOpusDecoder() {
      if (!opusDecoder) {
		   int error = 0;
		   opusDecoder = opus_decoder_create(DECODE_PCM_SAMPLERATE,DECODE_PCM_CHANNEL_COUNT,&error);
		   opus_decoder_ctl(opusDecoder, OPUS_SET_GAIN(1024));

            MS_DEBUG_TAG(info, "Opus Decoder opened");
	   } else {
           MS_DEBUG_TAG(info, "Opus Decoder already opened, no need open again");
       }
    }

    void AudioPCMObserver::CloseOpusDecoder() {
        if (opusDecoder) {
            opus_decoder_destroy(opusDecoder);
            opusDecoder = NULL;
             MS_DEBUG_TAG(info, "Opus Decoder closed");
        } else {
            MS_DEBUG_TAG(info, "Opus Decoder already closed, no need close again");
        }
    }

} // namespace RTC





#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))


#define BASE64_PAD '='
#define BASE64DE_FIRST '+'
#define BASE64DE_LAST 'z'

/* BASE 64 encode table */
static const char base64en[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

/* ASCII order for BASE 64 decode, 255 in unused character */
static const unsigned char base64de[] = {
	/* nul, soh, stx, etx, eot, enq, ack, bel, */
	   255, 255, 255, 255, 255, 255, 255, 255,

	/*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
	   255, 255, 255, 255, 255, 255, 255, 255,

	/* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
	   255, 255, 255, 255, 255, 255, 255, 255,

	/* can,  em, sub, esc,  fs,  gs,  rs,  us, */
	   255, 255, 255, 255, 255, 255, 255, 255,

	/*  sp, '!', '"', '#', '$', '%', '&', ''', */
	   255, 255, 255, 255, 255, 255, 255, 255,

	/* '(', ')', '*', '+', ',', '-', '.', '/', */
	   255, 255, 255,  62, 255, 255, 255,  63,

	/* '0', '1', '2', '3', '4', '5', '6', '7', */
	    52,  53,  54,  55,  56,  57,  58,  59,

	/* '8', '9', ':', ';', '<', '=', '>', '?', */
	    60,  61, 255, 255, 255, 255, 255, 255,

	/* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
	   255,   0,   1,  2,   3,   4,   5,    6,

	/* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
	     7,   8,   9,  10,  11,  12,  13,  14,

	/* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
	    15,  16,  17,  18,  19,  20,  21,  22,

	/* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
	    23,  24,  25, 255, 255, 255, 255, 255,

	/* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
	   255,  26,  27,  28,  29,  30,  31,  32,

	/* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
	    33,  34,  35,  36,  37,  38,  39,  40,

	/* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
	    41,  42,  43,  44,  45,  46,  47,  48,

	/* 'x', 'y', 'z', '{', '|', '}', '~', del, */
	    49,  50,  51, 255, 255, 255, 255, 255
};

unsigned int
base64_encode(const unsigned char *in, unsigned int inlen, char *out)
{
	int s;
	unsigned int i;
	unsigned int j;
	unsigned char c;
	unsigned char l;

	s = 0;
	l = 0;
	for (i = j = 0; i < inlen; i++) {
		c = in[i];

		switch (s) {
		case 0:
			s = 1;
			out[j++] = base64en[(c >> 2) & 0x3F];
			break;
		case 1:
			s = 2;
			out[j++] = base64en[((l & 0x3) << 4) | ((c >> 4) & 0xF)];
			break;
		case 2:
			s = 0;
			out[j++] = base64en[((l & 0xF) << 2) | ((c >> 6) & 0x3)];
			out[j++] = base64en[c & 0x3F];
			break;
		}
		l = c;
	}

	switch (s) {
	case 1:
		out[j++] = base64en[(l & 0x3) << 4];
		out[j++] = BASE64_PAD;
		out[j++] = BASE64_PAD;
		break;
	case 2:
		out[j++] = base64en[(l & 0xF) << 2];
		out[j++] = BASE64_PAD;
		break;
	}

	out[j] = 0;

	return j;
}

unsigned int
base64_decode(const char *in, unsigned int inlen, unsigned char *out)
{
	unsigned int i;
	unsigned int j;
	unsigned char c;

	if (inlen & 0x3) {
		return 0;
	}

	for (i = j = 0; i < inlen; i++) {
		if (in[i] == BASE64_PAD) {
			break;
		}
		if (in[i] < BASE64DE_FIRST || in[i] > BASE64DE_LAST) {
			return 0;
		}

		c = base64de[(unsigned char)in[i]];
		if (c == 255) {
			return 0;
		}

		switch (i & 0x3) {
		case 0:
			out[j] = (c << 2) & 0xFF;
			break;
		case 1:
			out[j++] |= (c >> 4) & 0x3;
			out[j] = (c & 0xF) << 4; 
			break;
		case 2:
			out[j++] |= (c >> 2) & 0xF;
			out[j] = (c & 0x3) << 6;
			break;
		case 3:
			out[j++] |= c;
			break;
		}
	}

	return j;
}