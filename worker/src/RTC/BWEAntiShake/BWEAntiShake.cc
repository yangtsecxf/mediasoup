#include "BWEAntiShake.h"

short BWEAntiShake::get_last_layer()
{
	return last_layer_;
}

short BWEAntiShake::get_go_up_cnt()
{
	return go_up_cnt;
}

bool BWEAntiShake::is_shake_layer(short& layer)
{
	// 第一次允许
	if (last_layer_ <= -2)
	{
		last_layer_ = layer;
		go_up_cnt = 0;
		return false;
	}

	// 相等
	if (layer == last_layer_)
	{
		go_up_cnt = 0;
		return true;
	}

	//  速降
	if (layer < last_layer_)
	{
		last_layer_ = layer;
		go_up_cnt = 0;
		return false;
	}

	// 缓升
	if (layer > last_layer_)
	{
		// -1往高了切是可以的，不然久久不能进入画面
		if (-1 == last_layer_)
		{
			last_layer_ = layer;
			go_up_cnt = 0;
			return false;
		}

		// 缓升 计次2+
		go_up_cnt++;
		if (go_up_cnt >= 2)
		{
			// 升也不能跳着升，只能基于当前+1，且不能超过上限
			layer = last_layer_ + 1;

			last_layer_ = layer;
			go_up_cnt = 0;
			return false;
		}
	}

	// 如果确认layer是抖动的，那必须重置它为当前layer
	layer = last_layer_;
	return true;
}
