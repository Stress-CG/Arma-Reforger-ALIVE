class AliveProfileManager : Managed
{
	protected static const float RESERVATION_TIMEOUT_SECONDS = 10.0;
	protected static const float SPAWN_COOLDOWN_SECONDS = 20.0;
	protected static const float RECLAIM_COOLDOWN_SECONDS = 15.0;
	protected static const float ENGAGEMENT_GRACE_SECONDS = 45.0;
	protected static const float VISIBILITY_GRACE_SECONDS = 12.0;

	protected ref map<string, ref AliveProfileRuntimeRecord> m_mRuntimeByProfileId = new map<string, ref AliveProfileRuntimeRecord>();
	protected ref array<ref AliveProfileBinding> m_aBindings = new array<ref AliveProfileBinding>();

	void Reset()
	{
		m_mRuntimeByProfileId.Clear();
		m_aBindings.Clear();
	}

	AliveProfileRuntimeRecord EnsureRuntimeRecord(ARAL_ForceProfile profile, float now)
	{
		AliveProfileRuntimeRecord runtimeRecord = m_mRuntimeByProfileId.Get(profile.m_sId);
		if (runtimeRecord)
			return runtimeRecord;

		runtimeRecord = new AliveProfileRuntimeRecord();
		runtimeRecord.m_sProfileId = profile.m_sId;
		runtimeRecord.m_fStateChangedAt = now;
		m_mRuntimeByProfileId.Set(profile.m_sId, runtimeRecord);
		return runtimeRecord;
	}

	void RefreshRuntime(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		if (!runtimeRecord)
			return;

		if (runtimeRecord.m_bReservationHeld && runtimeRecord.m_fReservationUntil > 0 && now >= runtimeRecord.m_fReservationUntil)
			RollbackSpawnReservation(runtimeRecord, now);

		if (runtimeRecord.m_eHandoffState == AliveEHandoffState.Destroyed && profile && profile.m_iStrength > 0 && profile.m_eState == ARAL_EProfileState.VIRTUAL)
			SetState(runtimeRecord, AliveEHandoffState.Virtual, now);
	}

	AliveProfileBinding FindBindingByProfileId(string profileId)
	{
		foreach (AliveProfileBinding binding : m_aBindings)
		{
			if (binding.m_sProfileId == profileId)
				return binding;
		}

		return null;
	}

	AliveProfileBinding FindBindingByEntity(IEntity entity)
	{
		if (!entity)
			return null;

		foreach (AliveProfileBinding binding : m_aBindings)
		{
			if (binding.m_Entity == entity)
				return binding;
		}

		return null;
	}

	AliveProfileRuntimeRecord FindRuntimeRecord(string profileId)
	{
		return m_mRuntimeByProfileId.Get(profileId);
	}

	void GetBindingsSnapshot(out array<ref AliveProfileBinding> outBindings)
	{
		outBindings.Clear();
		foreach (AliveProfileBinding binding : m_aBindings)
			outBindings.Insert(binding);
	}

	AliveRelevanceContext BuildRelevance(ARAL_ForceProfile profile, ARAL_MissionPreset preset, AliveProfileRuntimeRecord runtimeRecord, array<vector> playerObservers, float now)
	{
		AliveRelevanceContext relevance = new AliveRelevanceContext();
		relevance.m_vBestSpawnOrigin = profile.m_vVirtualPosition;

		if (!playerObservers || playerObservers.IsEmpty())
			return relevance;

		float spawnRadius = preset.m_ActivationBudget.m_fInfantrySpawnRadius;
		if (profile.m_bVehicleProfile)
			spawnRadius = preset.m_ActivationBudget.m_fVehicleSpawnRadius;

		float holdRadius = Math.Max(spawnRadius + 250.0, preset.m_ActivationBudget.m_fDespawnRadius);
		float spawnRadiusSq = spawnRadius * spawnRadius;
		float holdRadiusSq = holdRadius * holdRadius;

		foreach (vector observer : playerObservers)
		{
			float distanceSq = vector.DistanceSq(profile.m_vVirtualPosition, observer);
			if (distanceSq < relevance.m_fClosestDistanceSq)
			{
				relevance.m_fClosestDistanceSq = distanceSq;
				relevance.m_vBestSpawnOrigin = observer;
			}
		}

		relevance.m_bSpawnRelevant = relevance.m_fClosestDistanceSq <= spawnRadiusSq;
		relevance.m_bStillRelevant = relevance.m_fClosestDistanceSq <= holdRadiusSq;
		relevance.m_bRecentlyVisible = runtimeRecord.m_fLastVisibleAt > 0 && now < runtimeRecord.m_fLastVisibleAt + VISIBILITY_GRACE_SECONDS;
		relevance.m_bRecentlyEngaged = runtimeRecord.m_fLastEngagedAt > 0 && now < runtimeRecord.m_fLastEngagedAt + ENGAGEMENT_GRACE_SECONDS;
		relevance.m_bPlayerVisible = relevance.m_bStillRelevant && relevance.m_bRecentlyVisible;
		return relevance;
	}

	bool TryReserveForSpawn(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		if (runtimeRecord.m_bReservationHeld)
			return false;

		if (runtimeRecord.m_eHandoffState != AliveEHandoffState.Virtual)
			return false;

		if (runtimeRecord.m_fSpawnCooldownUntil > now)
			return false;

		runtimeRecord.m_bReservationHeld = true;
		runtimeRecord.m_fReservationUntil = now + RESERVATION_TIMEOUT_SECONDS;
		runtimeRecord.m_iSpawnGeneration++;
		SetState(runtimeRecord, AliveEHandoffState.PendingSpawn, now);
		return true;
	}

	void MarkSpawnStarted(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		SetState(runtimeRecord, AliveEHandoffState.Spawning, now);
	}

	void RollbackSpawnReservation(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_bReservationHeld = false;
		runtimeRecord.m_fReservationUntil = 0;
		runtimeRecord.m_fSpawnCooldownUntil = now + 5.0;
		SetState(runtimeRecord, AliveEHandoffState.Virtual, now);
	}

	void CompleteSpawn(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, AliveProfileBinding binding, float now)
	{
		runtimeRecord.m_bReservationHeld = false;
		runtimeRecord.m_fReservationUntil = 0;
		runtimeRecord.m_fStateChangedAt = now;
		binding.m_eState = AliveEHandoffState.Physical;
		if (profile)
		{
			profile.m_iMaterializedCount = profile.m_iStrength;
			profile.m_fLastPhysicalUpdateAt = now;
		}

		SetState(runtimeRecord, AliveEHandoffState.Physical, now);
	}

	bool CanBeginReclaim(AliveProfileRuntimeRecord runtimeRecord, AliveRelevanceContext relevance, float now)
	{
		if (runtimeRecord.m_eHandoffState != AliveEHandoffState.Physical)
			return false;

		if (runtimeRecord.m_fReclaimCooldownUntil > now)
			return false;

		if (runtimeRecord.m_fNoReclaimUntil > now)
			return false;

		return relevance.CanReclaim();
	}

	void BeginReclaim(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		SetState(runtimeRecord, AliveEHandoffState.PendingReclaim, now);
	}

	void MarkReclaiming(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		SetState(runtimeRecord, AliveEHandoffState.Reclaiming, now);
	}

	void CompleteReclaim(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_fReclaimCooldownUntil = now + RECLAIM_COOLDOWN_SECONDS;
		runtimeRecord.m_fSpawnCooldownUntil = now + SPAWN_COOLDOWN_SECONDS;
		runtimeRecord.m_bReservationHeld = false;
		runtimeRecord.m_fReservationUntil = 0;
		if (profile)
			profile.m_iMaterializedCount = 0;

		SetState(runtimeRecord, AliveEHandoffState.Virtual, now);
	}

	void MarkDestroyed(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_bReservationHeld = false;
		runtimeRecord.m_fReservationUntil = 0;
		if (profile)
			profile.m_iMaterializedCount = 0;

		SetState(runtimeRecord, AliveEHandoffState.Destroyed, now);
	}

	void RecoverLostProjection(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_bReservationHeld = false;
		runtimeRecord.m_fReservationUntil = 0;
		runtimeRecord.m_fSpawnCooldownUntil = now + 5.0;
		if (profile)
			profile.m_iMaterializedCount = 0;

		SetState(runtimeRecord, AliveEHandoffState.Virtual, now);
	}

	void MarkObserved(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_fLastVisibleAt = now;
	}

	void MarkEngaged(AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		runtimeRecord.m_fLastEngagedAt = now;
		runtimeRecord.m_fNoReclaimUntil = now + ENGAGEMENT_GRACE_SECONDS;
	}

	void SetTacticalRuntimeState(AliveProfileRuntimeRecord runtimeRecord, AliveTacticalUpdateSnapshot tacticalUpdate)
	{
		if (!runtimeRecord || !tacticalUpdate)
			return;

		runtimeRecord.m_eLastTacticalMode = tacticalUpdate.m_eTacticalMode;
		runtimeRecord.m_fEstimatedSpeedMps = tacticalUpdate.m_fEstimatedSpeedMps;
		runtimeRecord.m_bVehicleMoving = tacticalUpdate.m_bVehicleMoving;
		runtimeRecord.m_bPlayerOccupancyBlocked = tacticalUpdate.m_bPlayerOccupancyBlocked;
		runtimeRecord.m_bHasEnemyTarget = tacticalUpdate.m_bHasEnemyTarget;
		runtimeRecord.m_bFiredRecently = tacticalUpdate.m_bFiredRecently;
		runtimeRecord.m_bTookDamageRecently = tacticalUpdate.m_bTookDamageRecently;
		runtimeRecord.m_sLastReclaimBlockReason = tacticalUpdate.m_sReclaimBlockReason;
	}

	void SetReclaimBlockReason(AliveProfileRuntimeRecord runtimeRecord, string reason)
	{
		if (!runtimeRecord)
			return;

		runtimeRecord.m_sLastReclaimBlockReason = reason;
	}

	AliveSpawnSpec BuildSpawnSpec(ARAL_ForceProfile profile, ARAL_Order activeOrder, AliveProfileRuntimeRecord runtimeRecord)
	{
		AliveSpawnSpec spawnSpec = new AliveSpawnSpec();
		spawnSpec.m_sProfileId = profile.m_sId;
		spawnSpec.m_sFactionKey = profile.m_sFactionKey;
		spawnSpec.m_sTemplateId = profile.m_sTemplateId;
		spawnSpec.m_sDisplayName = profile.m_sDisplayName;
		spawnSpec.m_iDesiredCount = profile.m_iStrength;
		spawnSpec.m_bVehicleProfile = profile.m_bVehicleProfile;
		spawnSpec.m_iSpawnGeneration = runtimeRecord.m_iSpawnGeneration;
		if (activeOrder)
			spawnSpec.m_sOrderId = activeOrder.m_sId;

		spawnSpec.m_mTransform[3] = profile.m_vVirtualPosition;

		return spawnSpec;
	}

	AliveProfileBinding CreateBinding(string profileId, IEntity entity, int spawnGeneration, float now, bool rawProjectionFallback = false)
	{
		AliveProfileBinding existing = FindBindingByProfileId(profileId);
		if (existing)
			RemoveBinding(profileId);

		AliveProfileBinding binding = new AliveProfileBinding();
		binding.m_sProfileId = profileId;
		binding.m_Entity = entity;
		binding.m_EntityId = entity.GetID();
		binding.m_iSpawnGeneration = spawnGeneration;
		binding.m_fCreatedAt = now;
		binding.m_bRawProjectionFallback = rawProjectionFallback;
		binding.m_eState = AliveEHandoffState.Physical;

		RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(entity);
		if (rplComponent)
			binding.m_RplId = rplComponent.Id();

		m_aBindings.Insert(binding);
		return binding;
	}

	void RemoveBinding(string profileId)
	{
		for (int i = m_aBindings.Count() - 1; i >= 0; i--)
		{
			if (m_aBindings[i].m_sProfileId != profileId)
				continue;

			m_aBindings.Remove(i);
		}
	}

	void WriteBackSnapshot(ARAL_ForceProfile profile, ARAL_Order activeOrder, AlivePhysicalSnapshot snapshot)
	{
		if (!profile || !snapshot)
			return;

		profile.m_vVirtualPosition = snapshot.m_vPosition;
		profile.m_iStrength = Math.Max(snapshot.m_iAliveCount, 0);
		profile.m_iMaterializedCount = 0;
		profile.m_fLastPhysicalUpdateAt = snapshot.m_fCapturedAt;
		profile.m_bVehicleOperational = snapshot.m_VehicleState.m_bOperational;
		profile.m_fVehicleHealthNormalized = snapshot.m_VehicleState.m_fHealthNormalized;
		profile.m_fVehicleFuelNormalized = snapshot.m_VehicleState.m_fFuelNormalized;

		if (!activeOrder)
			return;

		activeOrder.m_fProgressNormalized = snapshot.m_OrderProgress.m_fCompletionNormalized;
		activeOrder.m_vLastKnownPosition = snapshot.m_OrderProgress.m_vAnchorPosition;
		activeOrder.m_fLastPhysicalUpdateAt = snapshot.m_fCapturedAt;
	}

	int GetBindingCount()
	{
		return m_aBindings.Count();
	}

	int CountRawBindings()
	{
		int count = 0;
		foreach (AliveProfileBinding binding : m_aBindings)
		{
			if (binding.m_bRawProjectionFallback)
				count++;
		}

		return count;
	}

	int CountProfilesInState(AliveEHandoffState state)
	{
		int count = 0;
		foreach (string profileId, AliveProfileRuntimeRecord runtimeRecord : m_mRuntimeByProfileId)
		{
			if (runtimeRecord.m_eHandoffState == state)
				count++;
		}

		return count;
	}

	protected void SetState(AliveProfileRuntimeRecord runtimeRecord, AliveEHandoffState newState, float now)
	{
		runtimeRecord.m_eHandoffState = newState;
		runtimeRecord.m_fStateChangedAt = now;
	}
}
