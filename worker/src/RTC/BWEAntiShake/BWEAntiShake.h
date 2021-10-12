#pragma once


class BWEAntiShake
{
private:
	short last_layer_ = -2;
	short go_up_cnt = 0;

public:
	BWEAntiShake(){}
	virtual ~BWEAntiShake(){}

	short get_last_layer();

	short get_go_up_cnt();

	bool is_shake_layer(short& layer);

};