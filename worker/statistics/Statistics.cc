#include "Statistics.h"
#include "util.h"
#include "log.h"
using namespace std;

const int time_delta_dump_interval = 3000;
const int bitrate_dump_interval = 120;
const int spatial_layer_dump_interval = 3;
#ifdef WIN32
#define FOLDER_PATH "D://qz//cst-media-server//node_modules//mediasoup//test//statistics//"
#else
#define FOLDER_PATH "//Users//cxf//qz//cst-media-server//node_modules//mediasoup//test//statistics//"
#endif


Statistics::Statistics()
{
	init_time_ms_ = util::get_now_ms();
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
	init_transport_data(tid);

	if (delta > 0)
	{
		datas_[tid].str_time_deltas += (std::to_string(delta) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
		datas_[tid].str_time_deltas += ",";
		++datas_[tid].time_deltas_cnt;
	}

// 	if (datas_[tid].time_deltas_cnt > time_delta_dump_interval)
// 	{
// 		dump_time_delta();
// 	}
}

void Statistics::update_bitrate(const uint32_t availableBitrate, const std::string& tid)
{
	datas_[tid].str_bitrate_ += (std::to_string(availableBitrate / 1024) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
	datas_[tid].str_bitrate_ += ",";
	++datas_[tid].bitrate_cnt_;

 	if (datas_[tid].bitrate_cnt_ > bitrate_dump_interval)
 	{
 		//dump_bitrate();
		//dump_all(tid);
 	}
}

void Statistics::update_spatial_layer(const int spatial_layer, const std::string& tid)
{
	datas_[tid].str_target_layers += std::string(std::to_string(spatial_layer) + "-" + std::to_string(util::get_now_ms()-init_time_ms_));
	datas_[tid].str_target_layers += ",";
	++datas_[tid].target_layers_cnt;

	if (datas_[tid].target_layers_cnt > spatial_layer_dump_interval)
	{
		//dump_spatial_layer();
		//dump_all(tid);
	}
}

void Statistics::dump_time_delta(const std::string& tid)
{
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_delta.txt").c_str(), (void*)datas_[tid].str_time_deltas.c_str(), datas_[tid].str_time_deltas.length());
	datas_[tid].time_deltas_cnt = 0;
	datas_[tid].str_time_deltas = "";
}

void Statistics::dump_bitrate(const std::string& tid)
{
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:" + tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_bitrate.txt").c_str(), (void*)datas_[tid].str_bitrate_.c_str(), datas_[tid].str_bitrate_.length());
	datas_[tid].str_bitrate_ = "";
	datas_[tid].bitrate_cnt_ = 0;
}

void Statistics::dump_spatial_layer(const std::string& tid)
{
	CHECK_VOID(datas_.find(tid) != datas_.end(), string("has no tid:"+tid));

	util::write_file(std::string(FOLDER_PATH + tid + "_spatial_layers.txt").c_str(), (void*)datas_[tid].str_target_layers.c_str(), datas_[tid].str_target_layers.length());
	datas_[tid].target_layers_cnt = 0;
	datas_[tid].str_target_layers = "";
}

void Statistics::dump_all(const std::string& tid)
{
	dump_time_delta(tid);
	dump_bitrate(tid);
	dump_spatial_layer(tid);
}

void Statistics::init_transport_data(const std::string& tid)
{
	if (datas_.find(tid) == datas_.end())
	{
		datas_[tid] = StatisticsData{};
	}
}