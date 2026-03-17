#include "Renderer.h"
#include "Walnut/Random.h"

#include <execution>

namespace Utils {
	static float Pi = 3.14159265359f;

	static void BuildOrthonormalBasis(glm::vec3 N, glm::vec3& T, glm::vec3& B)
	{
		glm::vec3 up = glm::abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		T = glm::normalize(glm::cross(up, N));
		B = glm::cross(N, T);
	}

	static glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness)
	{
		float a = roughness * roughness;
		float phi = 2.0f * Pi * Xi.x;
		float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
		float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

		glm::vec3 H;
		H.x = cos(phi) * sinTheta;
		H.y = sin(phi) * sinTheta;
		H.z = cosTheta;

		glm::vec3 T, B;
		BuildOrthonormalBasis(N, T, B);

		return T * H.x + B * H.y + N * H.z;
	}

	float GeometrySchlickGGX(float NdotV, float roughness)
	{
		float r = roughness + 1.0f;
		float k = r * r / 8.0f;

		float nom = NdotV;
		float denom = NdotV * (1.0f - k) + k;

		return nom / denom;
	}

	float GeometrySmith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness)
	{
		float NdotV = glm::max(glm::dot(N, V), 0.0f);
		float NdotL = glm::max(glm::dot(N, L), 0.0f);

		float ggx2 = GeometrySchlickGGX(NdotV, roughness);
		float ggx1 = GeometrySchlickGGX(NdotL, roughness);

		return ggx1 * ggx2;
	}

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(
			RandomFloat(seed)*2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f));
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulateColor /= (float)m_FrameIndex;

					if (m_Settings.PhysicalCamera)
					{
						glm::vec4 finalColor = accumulateColor;

						finalColor.r = finalColor.r / (finalColor.r + 1.0f);
						finalColor.g = finalColor.g / (finalColor.g + 1.0f);
						finalColor.b = finalColor.b / (finalColor.b + 1.0f);

						finalColor.r = pow(finalColor.r, 1.0f / 2.2f);
						finalColor.g = pow(finalColor.g, 1.0f / 2.2f);
						finalColor.b = pow(finalColor.b, 1.0f / 2.2f);

						accumulateColor = glm::clamp(finalColor, glm::vec4(0.0f), glm::vec4(1.0f));
					}
					else
					{
						accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));
					}
					
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulateColor);
				});
		});

#else

	for(uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulateColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulateColor /= (float)m_FrameIndex;

			if (m_Settings.PhysicalCamera)
			{
				glm::vec4 finalColor = accumulateColor;

				finalColor.r = finalColor.r / (finalColor.r + 1.0f);
				finalColor.g = finalColor.g / (finalColor.g + 1.0f);
				finalColor.b = finalColor.b / (finalColor.b + 1.0f);

				finalColor.r = pow(finalColor.r, 1.0f / 2.2f);
				finalColor.g = pow(finalColor.g, 1.0f / 2.2f);
				finalColor.b = pow(finalColor.b, 1.0f / 2.2f);

				accumulateColor = glm::clamp(finalColor, glm::vec4(0.0f), glm::vec4(1.0f));
			}
			else
			{
				accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));
			}

			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulateColor);
		}
	}

#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);
	glm::vec3 SkyColor = m_ActiveScene->SkyColor;

	if (m_Settings.PhysicalCamera)
	{
		SkyColor *= 3.0f;
	}

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;
	int bounces = 5; 

	switch (m_Settings.Modes)
	{
	case ERayTracingMode::E_luminousMaterial:
	{
		for (int i = 0; i < bounces; i++)
		{
			seed += i;

			Renderer::HitPayload payload = TraceRay(ray);

			if (payload.HitDistance < 0.0f)
			{
				break;
			}

			const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
			const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

			light += material.GetEmission();

			ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;

			if (m_Settings.SlowRandom)
				ray.Direction = glm::normalize(payload.WorldNormal + Walnut::Random::InUnitSphere());
			else
				ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
		}
		break;
	}
		
	case ERayTracingMode::E_PBRPathTracing:
	{
		for (int i = 0; i < bounces; i++)
		{
			Renderer::HitPayload payload = TraceRay(ray);

			if (payload.HitDistance < 0.0f)
			{
				light += SkyColor * contribution;
				break;
			}

			const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
			const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

			light += material.GetEmission() * contribution;

		
			glm::vec2 Xi = { Utils::RandomFloat(seed), Utils::RandomFloat(seed) };

			//菲涅尔
			float cosTheta = glm::clamp(glm::dot(payload.WorldNormal, -ray.Direction), 0.0f, 1.0f);
			//基础反射率F0: 非金属固定0.04，金属使用Albedo
			glm::vec3 F0 = glm::mix(glm::vec3(0.04f), material.Albedo, material.Metallic);
			glm::vec3 F = F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);

			float specularProb = (F.r + F.g + F.b) / 3.0f;
			//如果是金属，强制大概率反射（金属没有漫反射）
			specularProb = glm::mix(specularProb, 1.0f, material.Metallic);

			//俄罗斯轮盘赌
			if (Utils::RandomFloat(seed) < specularProb)
			{
				glm::vec3 H = Utils::ImportanceSampleGGX(Xi, payload.WorldNormal, material.Roughness);
				glm::vec3 specularDir = glm::reflect(ray.Direction, H);

				if (glm::dot(specularDir, payload.WorldNormal) > 0.0f)
				{
					float G = Utils::GeometrySmith(payload.WorldNormal, -ray.Direction, specularDir, material.Roughness);

					float NdotV = glm::clamp(glm::dot(payload.WorldNormal, -ray.Direction), 0.001f, 1.0f); // 防止除0
					float NdotH = glm::clamp(glm::dot(payload.WorldNormal, H), 0.001f, 1.0f);
					float VdotH = glm::clamp(glm::dot(-ray.Direction, H), 0.001f, 1.0f);

					float weight = G * VdotH / (NdotV * NdotH);

					contribution *= F * weight;
					contribution /= (specularProb + 0.001f);
				}
				else
				{
					contribution = glm::vec3(0.0f);
					break;
				}
				
				ray.Direction = specularDir;
			}
			else
			{
				ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));

				contribution *= material.Albedo;
				contribution /= (1.0f - specularProb + 0.001f);
			}

			ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		}
		break;
	    }
	}

	return glm::vec4(light, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];

		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2 * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminat = b * b - 4.0f * a * c;

		if (discriminat < 0.0f)
			continue;

		float closestT = (-b - glm::sqrt(discriminat)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			closestSphere = (int)i;
			hitDistance = closestT;
		}
	}

	if (closestSphere < 0)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int ObjectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = ObjectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[ObjectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}
