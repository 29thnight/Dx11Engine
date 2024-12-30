#pragma once
#include <wrl.h>
#include <exception>
#include "TypeDefinition.h"

namespace DirectX11
{
	class TimeSystem
	{
	public:
		TimeSystem() :
			m_elapsedTicks(0),
			m_totalTicks(0),
			m_leftOverTicks(0),
			m_frameCount(0),
			m_framesPerSecond(0),
			m_framesThisSecond(0),
			m_qpcSecondCounter(0),
			m_isFixedTimeStep(false),
			m_targetElapsedTicks(TicksPerSecond / 60)
		{
			if (!QueryPerformanceFrequency(&m_qpcFrequency))
			{
				throw std::exception("Failed_QueryPerformanceFrequency 24");
			}

			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw std::exception("Failed_QueryPerformanceCounter 29");
			}

			// �ִ� ��Ÿ�� 1���� 1/10�� �ʱ�ȭ�մϴ�.
			m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
		}

		//���� �����Ӱ� ���� ������ ������ �ð��� ��ȯ�մϴ�.
		uint64_t GetElapsedTicks() const { return m_elapsedTicks; }
		double GetElapsedSeconds() const { return TicksToSeconds(m_elapsedTicks); }

		//���α׷� ���ۺ����� �� �ð��� ��ȯ�մϴ�.
		uint64_t GetTotalTicks() const { return m_totalTicks; }
		double GetTotalSeconds() const { return TicksToSeconds(m_totalTicks); }

		//���� ������ ���� ��ȯ�մϴ�.
		uint32_t GetFrameCount() const { return m_frameCount; }

		//���� �ʴ� ������ ���� ��ȯ�մϴ�.
		uint32_t GetFramesPerSecond() const { return m_framesPerSecond; }

		// ���� timestep ��带 ������� ���� timestep ��带 ������� �����մϴ�.
		void SetFixedTimeStep(bool isFixedTimestep) { m_isFixedTimeStep = isFixedTimestep; }

		// ���� timestep ��忡���� Update ȣ�� �󵵸� �����ϼ���.
		void SetTargetElapsedTicks(uint64_t targetElapsed) { m_targetElapsedTicks = targetElapsed; }
		void SetTargetElapsedSeconds(double targetElapsed) { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

		// ���� ������ �ʴ� 10,000,000 ������ ����ϴ� �ð��� ��Ÿ���ϴ�.
		static const uint64 TicksPerSecond = 10000000;

		static double TicksToSeconds(uint64 ticks) { return static_cast<double>(ticks) / TicksPerSecond; }
		static uint64 SecondsToTicks(double seconds) { return static_cast<uint64>(seconds * TicksPerSecond); }

		// �ǵ����� Ÿ�̹� ���� ��(��: IO ���� �۾�)
		// �� �׸��� ȣ���Ͽ� catch-up ������ �õ��ϴ� ���� timestep ���� �����մϴ�.
		// Update ȣ���Դϴ�.

		void ResetElapsedTime()
		{
			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw std::exception("Failed_QueryPerformanceCounter 71");
			}
			m_leftOverTicks = 0;
			m_framesPerSecond = 0;
			m_framesThisSecond = 0;
			m_qpcSecondCounter = 0;
		}

		// ������ Update �Լ��� ������ Ƚ���� ȣ���Ͽ� Ÿ�̸� ���¸� ������Ʈ�մϴ�.
		template<typename TUpdate>
		void Tick(const TUpdate& update)
		{
			// ���� �ð��� �����մϴ�.
			LARGE_INTEGER currentTime;

			if (!QueryPerformanceCounter(&currentTime))
			{
				throw std::exception("Failed_QueryPerformanceCounter 88");
			}

			uint64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

			m_qpcLastTime = currentTime;
			m_qpcSecondCounter += timeDelta;

			// �ʹ� ū �ð� ��Ÿ�� �����մϴ�(��: ����ſ��� �Ͻ� ������ ��).
			if (timeDelta > m_qpcMaxDelta)
			{
				timeDelta = m_qpcMaxDelta;
			}

			// QPC ������ ���� ���� �������� ��ȯ�մϴ�. ������ �������� ���� �����÷��� �� �����ϴ�.
			timeDelta *= TicksPerSecond;
			timeDelta /= m_qpcFrequency.QuadPart;

			uint32 lastFrameCount = m_frameCount;

			if (m_isFixedTimeStep)
			{
				// ���� timestep ������Ʈ ���Դϴ�.

				// ���� ��ǥ ��� �ð�(1/4�и��� �̳�)�� �����ϰ� �����ϴ� ���
				// ��� ���� ��Ȯ�� ��ġ�ϵ��� Ŭ���� �����մϴ�. �̷��� �ϸ� ����ϰ� ������ ������ �����˴ϴ�.
				// ������ �� �ֽ��ϴ�. �ð��� �������� ������ 59.94 NTSC ���÷��̿��� ����ϵ��� ������
				// vsync�� �Բ� ����Ǹ� 60fps ���� ������Ʈ�� �䱸�Ǵ� ���ӿ�
				// ����� ������ �����Ǿ� �ᱹ �������� ������ �� �ֽ��ϴ�. �ݿø��ϴ� ���� �����ϴ�.
				// ���� ������ 0���� �ٿ� ��Ȱ�ϰ� ����� �� �ֵ��� �ϼ���.

				if (abs(static_cast<int64>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
				{
					timeDelta = m_targetElapsedTicks;
				}

				m_leftOverTicks += timeDelta;

				while (m_leftOverTicks >= m_targetElapsedTicks)
				{
					m_elapsedTicks = m_targetElapsedTicks;
					m_totalTicks += m_targetElapsedTicks;
					m_leftOverTicks -= m_targetElapsedTicks;
					m_frameCount++;

					update();
				}
			}
			else
			{
				// ���� timestep ������Ʈ ���Դϴ�.
				m_elapsedTicks = timeDelta;
				m_totalTicks += timeDelta;
				m_leftOverTicks = 0;
				m_frameCount++;

				update();
			}

			// ���� framerate�� �����մϴ�.
			if (m_frameCount != lastFrameCount)
			{
				m_framesThisSecond++;
			}

			if (m_qpcSecondCounter >= static_cast<uint64>(m_qpcFrequency.QuadPart))
			{
				m_framesPerSecond = m_framesThisSecond;
				m_framesThisSecond = 0;
				m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
			}
		}

	private:
		// ���� Ÿ�̹� �����Ϳ��� QPC ������ ���˴ϴ�.
		LARGE_INTEGER m_qpcFrequency;
		LARGE_INTEGER m_qpcLastTime;
		uint64 m_qpcMaxDelta;

		// �Ļ��� Ÿ�̹� �����Ϳ��� ���� ���� ������ ���˴ϴ�.
		uint64 m_elapsedTicks;
		uint64 m_totalTicks;
		uint64 m_leftOverTicks;

		// framerate ������ ����Դϴ�.
		uint32 m_frameCount;
		uint32 m_framesPerSecond;
		uint32 m_framesThisSecond;
		uint64 m_qpcSecondCounter;

		// ���� timestep ��� ������ ����Դϴ�.
		bool m_isFixedTimeStep;
		uint64 m_targetElapsedTicks;
	};
}