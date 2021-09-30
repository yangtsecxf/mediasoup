#pragma once
#include <string>
#include <map>

#define STS Statistics::ins()
class Statistics
{
private:
	struct StatisticsData
	{
		std::string str_target_layers;
		int64_t target_layers_cnt = 0;

		std::string str_bitrate_;
		int64_t bitrate_cnt_ = 0;

		std::string str_time_deltas;
		int64_t time_deltas_cnt = 0;
	};

	std::map<std::string, StatisticsData> datas_;

	int64_t init_time_ms_;

public:
	Statistics();
	~Statistics();

	static Statistics* ins();

	void update_time_delta(int16_t delta, const std::string& transportId);

	void update_bitrate(const uint32_t availableBitrate, const std::string& transportId);

	void update_spatial_layer(const int spatial_layer, const std::string& transportId);

	void dump_time_delta(const std::string& transportId);

	void dump_bitrate(const std::string& transportId);
	
	void dump_spatial_layer(const std::string& transportId);

	void dump_all(const std::string& transportId);

private:
	void init_transport_data(const std::string& transportId);

};