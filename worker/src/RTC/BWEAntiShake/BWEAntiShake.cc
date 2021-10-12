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
	// ��һ������
	if (last_layer_ <= -2)
	{
		last_layer_ = layer;
		go_up_cnt = 0;
		return false;
	}

	// ���
	if (layer == last_layer_)
	{
		go_up_cnt = 0;
		return true;
	}

	//  �ٽ�
	if (layer < last_layer_)
	{
		last_layer_ = layer;
		go_up_cnt = 0;
		return false;
	}

	// ����
	if (layer > last_layer_)
	{
		// -1���������ǿ��Եģ���Ȼ�þò��ܽ��뻭��
		if (-1 == last_layer_)
		{
			last_layer_ = layer;
			go_up_cnt = 0;
			return false;
		}

		// ���� �ƴ�2+
		go_up_cnt++;
		if (go_up_cnt >= 2)
		{
			// ��Ҳ������������ֻ�ܻ��ڵ�ǰ+1���Ҳ��ܳ�������
			layer = last_layer_ + 1;

			last_layer_ = layer;
			go_up_cnt = 0;
			return false;
		}
	}

	// ���ȷ��layer�Ƕ����ģ��Ǳ���������Ϊ��ǰlayer
	layer = last_layer_;
	return true;
}
