#pragma once
#include <string>
#include <map>

#define STS Statistics::ins()
class Statistics
{
private:
	struct StatisticsData
	{
		std::string peerId;
		std::string roomId;

		std::string str_target_layers;
		int64_t target_layers_cnt = 0;

		std::string str_bitrate;
		int64_t bitrate_cnt = 0;
		uint32_t available_bitrate = 0;

		std::string str_time_deltas;
		int64_t time_deltas_cnt = 0;

		std::string str_loss;
		int64_t loss_cnt = 0;

		std::string str_audio_score;
		int64_t audio_score_cnt = 0;
	};

	std::map<std::string, StatisticsData> datas_;															// map<transportId, StatisticsData>

	int64_t init_time_ms_;
	std::string cwd_;

	bool enable_statistics_;

public:
	Statistics();
	~Statistics();

	static Statistics* ins();

	void update_time_delta(int16_t delta, const std::string& transportId);

	void update_bitrate(const uint32_t availableBitrate, const std::string& transportId);

	void update_spatial_layer(const int spatial_layer, const std::string& transportId);

	void update_packet_loss(const float fraction_loss, const std::string& transportId);

	void update_audio_score(const int score, const std::string& transportId);

	uint32_t get_available_bitrate(const std::string& transportId);

	void dump_time_delta(const std::string& transportId);

	void dump_bitrate(const std::string& transportId);
	
	void dump_spatial_layer(const std::string& transportId);

	void dump_loss(const std::string& transportId);

	void dump_audio_score(const std::string& transportId);

	void dump_all(const std::string& transportId);

	void insert_room_info(const std::string& roomId, const std::string& peerId, const std::string& transportId);

	void remove(const std::string& transportId);

	void enable_statistics(bool enable_statistics);
};