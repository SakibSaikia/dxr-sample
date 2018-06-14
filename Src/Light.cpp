#include "stdafx.h"
#include "Light.h"

Light::Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness) :
	m_direction{ direction },
	m_color{ color },
	m_brightness{ brightness }
{
}

void Light::Fill(LightConstants* lightConst) const
{
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;
}
