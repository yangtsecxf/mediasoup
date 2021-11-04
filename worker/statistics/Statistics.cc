#include "Statistics.h"
#include "util.h"
#include "log.h"
using namespace std;

const int time_delta_dump_interval = 3000;
const int bitrate_dump_interval = 120;
const int spatial_layer_dump_interval = 3;

#if LINUX
#define FOLDER_PATH "//app//log//"
#elif 1
#define FOLDER_PATH "//Users//cxf//qz//cst-media-server//node_modules//mediasoup//test//statistics//data//"
#elif WIN
#define FOLDER_PATH "D://qz//cst-media-server//node_modules//mediasoup//test//statistics//data//"
#endif


Statistics::Statistics()
: enable_statistics_(true)
{
	init_time_ms_ = util::get_now_ms();
	cwd_ = util::get_current_working_dir() + "//mediasoup_bwe_statistics_data//";
	INFO("[cxf]cwd_:", cwd_);
}

Statistics::~Statistics()
{

}

Statistics* Statistics::ins()
{
	static Statistics s_o;
	return &s_o;
}

void Statistics::update_time_delta(int16_t delta, const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");

	if (delta > 0)
	{
		datas_[tid].str_time_deltas += (std::to_string(delta) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
		datas_[tid].str_time_deltas += ",";
		++datas_[tid].time_deltas_cnt;
	}
}

void Statistics::update_bitrate(const uint32_t availableBitrate, const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	INFO("peerid:", datas_[tid].peerId, "available:", (int)availableBitrate/1000);

	datas_[tid].str_bitrate += (std::to_string(availableBitrate/1000) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
	datas_[tid].str_bitrate += ",";
	++datas_[tid].bitrate_cnt;
	datas_[tid].available_bitrate = availableBitrate;
}

void Statistics::update_spatial_layer(const int spatial_layer, const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");

	datas_[tid].str_target_layers += std::string(std::to_string(spatial_layer) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
	datas_[tid].str_target_layers += ",";
	++datas_[tid].target_layers_cnt;
}

void Statistics::update_packet_loss(const float fraction_loss, const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");

	datas_[tid].str_loss += (std::to_string(fraction_loss*100) + "-" + std::to_string(util::get_now_ms() - init_time_ms_));
	datas_[tid].str_loss += ",";
	++datas_[tid].loss_cnt;
}

void Statistics::update_audio_score(const int score, const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	INFO("[cxf]update_audio_score:", std::to_string(score));

	datas_[tid].str_audio_score += (std::to_string(score) + "-" + std::to_string(util::get_now_ms() - init_time_ms_));
	datas_[tid].str_audio_score += ",";
	++datas_[tid].audio_score_cnt;
}

uint32_t Statistics::get_available_bitrate(const std::string& tid)
{
	return datas_[tid].available_bitrate;
}

std::string Statistics::get_peer_id(const std::string& tid)
{
	return datas_[tid].peerId;
}

void Statistics::dump_time_delta(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_" + datas_[tid].roomId + "_" + datas_[tid].peerId + "_delta.txt").c_str(), (void*)datas_[tid].str_time_deltas.c_str(), datas_[tid].str_time_deltas.length());

	datas_[tid].time_deltas_cnt = 0;
	datas_[tid].str_time_deltas = "";
}

void Statistics::dump_bitrate(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_" + datas_[tid].roomId + "_" + datas_[tid].peerId + "_bitrate.txt").c_str(), (void*)datas_[tid].str_bitrate.c_str(), datas_[tid].str_bitrate.length());

	datas_[tid].str_bitrate = "";
	datas_[tid].bitrate_cnt = 0;
}

void Statistics::dump_spatial_layer(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:"+tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_" + datas_[tid].roomId + "_" + datas_[tid].peerId + "_spatial_layers.txt").c_str(), (void*)datas_[tid].str_target_layers.c_str(), datas_[tid].str_target_layers.length());

	datas_[tid].target_layers_cnt = 0;
	datas_[tid].str_target_layers = "";
}

void Statistics::dump_loss(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_" + datas_[tid].roomId + "_" + datas_[tid].peerId + "_loss.txt").c_str(), (void*)datas_[tid].str_loss.c_str(), datas_[tid].str_loss.length());

	datas_[tid].str_loss = "";
	datas_[tid].loss_cnt = 0;
}

void Statistics::dump_audio_score(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_" + datas_[tid].roomId + "_" + datas_[tid].peerId + "_audio_score.txt").c_str(), (void*)datas_[tid].str_audio_score.c_str(), datas_[tid].str_audio_score.length());

	datas_[tid].str_audio_score = "";
	datas_[tid].audio_score_cnt = 0;
}

void Statistics::dump_all(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");

	dump_time_delta(tid);
	dump_bitrate(tid);
	dump_spatial_layer(tid);
	dump_loss(tid);
	dump_audio_score(tid);
}

void Statistics::insert_room_info(const std::string& roomId, const std::string& peerId, const std::string& transportId)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");
	INFO("[cxf] tid:", transportId, "rid:", roomId, "peerId:", peerId);

	datas_[transportId].peerId = peerId;
	datas_[transportId].roomId = roomId;
}

void Statistics::remove(const std::string& tid)
{
	CHECK_VOID(enable_statistics_, "enable statistics is false!");

	datas_.erase(tid);
}

void Statistics::enable_statistics(bool enable_statistics)
{
	enable_statistics_ = enable_statistics;
}