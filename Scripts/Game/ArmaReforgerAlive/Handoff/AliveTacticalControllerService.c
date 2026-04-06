class AliveProjectionMotionRecord : Managed
{
	vector m_vLastPosition = "0 0 0";
	float m_fLastSampleAt;
	int m_iLastKnownStrength = -1;
	float m_fLastKnownVehicleHealth = 1.0;
}

class AliveTacticalControllerService : Managed
{
	protected static const float VEHICLE_MOVING_THRESHOLD_MPS = 1.0;
	protected static const float PLAYER_OCCUPANCY_BLOCK_DISTANCE = 6.0;
	protected static const float HOSTILE_TARGET_DETECTION_RADIUS = 250.0;

	protected ref map<string, ref AliveProjectionMotionRecord> m_mMotionByProfileId = new map<string, ref AliveProjectionMotionRecord>();

	AliveTacticalUpdateSnapshot UpdateProjection(ARAL_WarState warState, ARAL_ForceProfile profile, ARAL_Order activeOrder, IEntity boundEntity, AlivePhysicalAgentComponent physicalAgent, array<vector> playerObservers, float now)
	{
		AliveTacticalUpdateSnapshot snapshot = new AliveTacticalUpdateSnapshot();
		if (!profile || !boundEntity)
			return snapshot;

		vector currentPosition = boundEntity.GetOrigin();
		AliveProjectionMotionRecord motionRecord = GetOrCreateMotionRecord(profile);
		snapshot.m_fEstimatedSpeedMps = CalculateEstimatedSpeed(motionRecord, currentPosition, now);
		snapshot.m_bVehicleMoving = profile.m_bVehicleProfile && snapshot.m_fEstimatedSpeedMps >= VEHICLE_MOVING_THRESHOLD_MPS;
		snapshot.m_bPlayerOccupancyBlocked = profile.m_bVehicleProfile && IsPlayerOccupancyBlocked(currentPosition, playerObservers);
		snapshot.m_bHasEnemyTarget = HasNearbyHostileTarget(warState, profile, currentPosition);
		snapshot.m_bTookDamageRecently = DetectDamageEvent(motionRecord, profile);
		snapshot.m_eTacticalMode = ResolveTacticalMode(profile, activeOrder, snapshot);

		snapshot.m_OrderProgress.m_vAnchorPosition = currentPosition;
		if (!activeOrder)
		{
			snapshot.m_sReclaimBlockReason = ResolveReclaimBlockReason(snapshot);
			ApplyPhysicalTacticalState(physicalAgent, snapshot);
			CommitMotionRecord(motionRecord, profile, currentPosition, now);
			return snapshot;
		}

		snapshot.m_OrderProgress.m_sOrderId = activeOrder.m_sId;

		ARAL_Objective objective = ARAL_WarStateUtility.FindObjectiveById(warState, activeOrder.m_sObjectiveId);
		if (!objective)
		{
			if (physicalAgent)
				physicalAgent.SetOrderProgress(activeOrder.m_fProgressNormalized, snapshot.m_OrderProgress.m_vAnchorPosition);
			snapshot.m_sReclaimBlockReason = ResolveReclaimBlockReason(snapshot);
			ApplyPhysicalTacticalState(physicalAgent, snapshot);
			CommitMotionRecord(motionRecord, profile, currentPosition, now);
			return snapshot;
		}

		snapshot.m_OrderProgress.m_vAnchorPosition = objective.m_vPosition;
		snapshot.m_OrderProgress.m_fCompletionNormalized = CalculateProgress(profile, objective, currentPosition);
		snapshot.m_bFiredRecently = ShouldTreatAsFiring(profile, activeOrder, objective, currentPosition, snapshot);
		snapshot.m_sReclaimBlockReason = ResolveReclaimBlockReason(snapshot);

		if (physicalAgent)
			physicalAgent.SetOrderProgress(snapshot.m_OrderProgress.m_fCompletionNormalized, snapshot.m_OrderProgress.m_vAnchorPosition);

		ApplyPhysicalTacticalState(physicalAgent, snapshot);
		CommitMotionRecord(motionRecord, profile, currentPosition, now);

		return snapshot;
	}

	bool IsProjectionEngaged(ARAL_WarState warState, ARAL_Order activeOrder, IEntity boundEntity)
	{
		if (!activeOrder || !boundEntity)
			return false;

		ARAL_Objective objective = ARAL_WarStateUtility.FindObjectiveById(warState, activeOrder.m_sObjectiveId);
		if (!objective)
			return false;

		float engagementRadius = Math.Max(objective.m_fRadius, 150.0);
		float distance = vector.Distance(boundEntity.GetOrigin(), objective.m_vPosition);
		return distance <= engagementRadius;
	}

	protected float CalculateProgress(ARAL_ForceProfile profile, ARAL_Objective objective, vector currentPosition)
	{
		float completionDistance = Math.Max(objective.m_fRadius, 50.0);
		float distanceToObjective = vector.Distance(currentPosition, objective.m_vPosition);
		if (distanceToObjective <= completionDistance)
			return 1.0;

		float normalizationDistance = Math.Max(completionDistance * 4.0, 300.0);
		float normalized = 1.0 - Math.Clamp(distanceToObjective / normalizationDistance, 0.0, 1.0);
		return normalized;
	}

	protected AliveProjectionMotionRecord GetOrCreateMotionRecord(ARAL_ForceProfile profile)
	{
		AliveProjectionMotionRecord motionRecord = m_mMotionByProfileId.Get(profile.m_sId);
		if (motionRecord)
			return motionRecord;

		motionRecord = new AliveProjectionMotionRecord();
		m_mMotionByProfileId.Set(profile.m_sId, motionRecord);
		return motionRecord;
	}

	protected float CalculateEstimatedSpeed(AliveProjectionMotionRecord motionRecord, vector currentPosition, float now)
	{
		if (!motionRecord || motionRecord.m_fLastSampleAt <= 0 || now <= motionRecord.m_fLastSampleAt)
			return 0;

		float timeDelta = now - motionRecord.m_fLastSampleAt;
		if (timeDelta <= 0)
			return 0;

		float distance = vector.Distance(currentPosition, motionRecord.m_vLastPosition);
		return distance / timeDelta;
	}

	protected void CommitMotionRecord(AliveProjectionMotionRecord motionRecord, ARAL_ForceProfile profile, vector currentPosition, float now)
	{
		if (!motionRecord || !profile)
			return;

		motionRecord.m_vLastPosition = currentPosition;
		motionRecord.m_fLastSampleAt = now;
		motionRecord.m_iLastKnownStrength = profile.m_iStrength;
		motionRecord.m_fLastKnownVehicleHealth = profile.m_fVehicleHealthNormalized;
	}

	protected bool DetectDamageEvent(AliveProjectionMotionRecord motionRecord, ARAL_ForceProfile profile)
	{
		if (!motionRecord || motionRecord.m_iLastKnownStrength < 0 || !profile)
			return false;

		if (profile.m_iStrength < motionRecord.m_iLastKnownStrength)
			return true;

		if (profile.m_fVehicleHealthNormalized < motionRecord.m_fLastKnownVehicleHealth)
			return true;

		return false;
	}

	protected bool IsPlayerOccupancyBlocked(vector currentPosition, array<vector> playerObservers)
	{
		if (!playerObservers)
			return false;

		foreach (vector playerObserver : playerObservers)
		{
			if (vector.Distance(playerObserver, currentPosition) <= PLAYER_OCCUPANCY_BLOCK_DISTANCE)
				return true;
		}

		return false;
	}

	protected bool HasNearbyHostileTarget(ARAL_WarState warState, ARAL_ForceProfile profile, vector currentPosition)
	{
		if (!warState || !profile)
			return false;

		foreach (ARAL_ForceProfile otherProfile : warState.m_aProfiles)
		{
			if (otherProfile.m_sId == profile.m_sId)
				continue;

			if (otherProfile.m_sFactionKey == profile.m_sFactionKey)
				continue;

			if (otherProfile.m_iStrength <= 0)
				continue;

			if (vector.Distance(otherProfile.m_vVirtualPosition, currentPosition) <= HOSTILE_TARGET_DETECTION_RADIUS)
				return true;
		}

		return false;
	}

	protected AliveETacticalMode ResolveTacticalMode(ARAL_ForceProfile profile, ARAL_Order activeOrder, AliveTacticalUpdateSnapshot snapshot)
	{
		if (!profile)
			return AliveETacticalMode.None;

		if (!activeOrder)
			return profile.m_bVehicleProfile ? AliveETacticalMode.MountedMovement : AliveETacticalMode.Hold;

		if (profile.m_bVehicleProfile)
		{
			switch (activeOrder.m_eType)
			{
				case ARAL_EOrderType.HOLD:
					return AliveETacticalMode.CombatPatrol;
				case ARAL_EOrderType.ATTACK:
					return snapshot.m_bHasEnemyTarget ? AliveETacticalMode.CombatPatrol : AliveETacticalMode.MountedMovement;
				case ARAL_EOrderType.DEFEND:
					return AliveETacticalMode.CombatPatrol;
				case ARAL_EOrderType.REDEPLOY:
					return AliveETacticalMode.MountedMovement;
				case ARAL_EOrderType.REINFORCE:
					return AliveETacticalMode.Transport;
			}
		}

		switch (activeOrder.m_eType)
		{
			case ARAL_EOrderType.HOLD:
				return AliveETacticalMode.Hold;
			case ARAL_EOrderType.ATTACK:
				return snapshot.m_bHasEnemyTarget ? AliveETacticalMode.LimitedAssault : AliveETacticalMode.Move;
			case ARAL_EOrderType.DEFEND:
				return AliveETacticalMode.Defend;
			case ARAL_EOrderType.REDEPLOY:
				return AliveETacticalMode.Move;
			case ARAL_EOrderType.REINFORCE:
				return AliveETacticalMode.Patrol;
		}

		return AliveETacticalMode.None;
	}

	protected bool ShouldTreatAsFiring(ARAL_ForceProfile profile, ARAL_Order activeOrder, ARAL_Objective objective, vector currentPosition, AliveTacticalUpdateSnapshot snapshot)
	{
		if (!profile || !activeOrder || !objective)
			return false;

		if (!snapshot.m_bHasEnemyTarget)
			return false;

		if (activeOrder.m_eType != ARAL_EOrderType.ATTACK && activeOrder.m_eType != ARAL_EOrderType.DEFEND)
			return false;

		float distanceToObjective = vector.Distance(currentPosition, objective.m_vPosition);
		return distanceToObjective <= Math.Max(objective.m_fRadius * 1.5, 200.0);
	}

	protected string ResolveReclaimBlockReason(AliveTacticalUpdateSnapshot snapshot)
	{
		if (!snapshot)
			return "";

		if (snapshot.m_bPlayerOccupancyBlocked)
			return "player-in-vehicle";
		if (snapshot.m_bVehicleMoving)
			return "vehicle-moving";
		if (snapshot.m_bTookDamageRecently)
			return "recent-damage";
		if (snapshot.m_bFiredRecently)
			return "recent-fire";
		if (snapshot.m_bHasEnemyTarget)
			return "enemy-target";
		return "";
	}

	protected void ApplyPhysicalTacticalState(AlivePhysicalAgentComponent physicalAgent, AliveTacticalUpdateSnapshot snapshot)
	{
		if (!physicalAgent || !snapshot)
			return;

		physicalAgent.SetTacticalState(
			snapshot.m_eTacticalMode,
			snapshot.m_fEstimatedSpeedMps,
			snapshot.m_bVehicleMoving,
			snapshot.m_bPlayerOccupancyBlocked,
			snapshot.m_bHasEnemyTarget,
			snapshot.m_bFiredRecently,
			snapshot.m_bTookDamageRecently
		);
	}
}
