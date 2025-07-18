#include "AnimatePosition.h"


namespace PositionAnimation
{
	int	 runningAnimations = 0, pausedAnimations = 0, animationDuration = 250;

	void update_state( QAbstractAnimation::State ns, QAbstractAnimation::State os )
	{
		switch ( os )
		{
			case QAbstractAnimation::Stopped:
				switch ( ns )
				{
					case QAbstractAnimation::Paused: ++pausedAnimations;
					case QAbstractAnimation::Running: ++runningAnimations; break;
				}
				break;
			case QAbstractAnimation::Paused:
				switch ( ns )
				{
					case QAbstractAnimation::Stopped: --runningAnimations;
					case QAbstractAnimation::Running: --pausedAnimations; break;
				}
				break;
			case QAbstractAnimation::Running:
				switch ( ns )
				{
					case QAbstractAnimation::Stopped: --runningAnimations; break;
					case QAbstractAnimation::Paused: ++pausedAnimations; break;
				}
				break;
		}
		// qDebug() << "AnimState: running=" << runningAnimations << ", paused=" <<
		// pausedAnimations;
	}
} // namespace PositionAnimation
