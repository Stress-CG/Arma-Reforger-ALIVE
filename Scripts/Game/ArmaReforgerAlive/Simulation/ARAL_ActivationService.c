class ARAL_ActivationService : Managed
{
	protected ref array<ref ARAL_ProfileActivationChange> m_aQueue = new array<ref ARAL_ProfileActivationChange>();
	protected float m_fDispatchDelay;
	protected static const float SOURCE_DEDUPE_DISTANCE = 30.0;

	void Step(ARAL_WarState warState, ARAL_ActivationBudget budget, float timeSlice, array<vector> spawnSources)
	{
		if (!warState || !budget)
			return;

		m_fDispatchDelay -= timeSlice;
		array<vector> uniqueSources = {};
		BuildUniqueSpawnSources(spawnSources, uniqueSources);
		RebuildQueue(warState, budget, uniqueSources);

		if (m_fDispatchDelay > 0 || m_aQueue.IsEmpty())
			return;

		ARAL_ProfileActivationChange nextChange = m_aQueue[0];
		m_aQueue.Remove(0);

		ARAL_ForceProfile profile = ARAL_WarStateUtility.FindProfileById(warState, nextChange.m_sProfileId);
		if (!profile)
			return;

		profile.m_eState = nextChange.m_eTargetState;
		profile.m_iMaterializedCount = 0;
		m_fDispatchDelay = budget.m_fSmoothSpawnSeconds;
	}

	protected void RebuildQueue(ARAL_WarState warState, ARAL_ActivationBudget budget, array<vector> spawnSources)
	{
		m_aQueue.Clear();

		int activeProfiles = ARAL_WarStateUtility.CountProfilesInState(warState, ARAL_EProfileState.MATERIALIZED);
		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			float closestDistanceSq = GetClosestSourceDistanceSq(profile.m_vVirtualPosition, spawnSources);

			float spawnRadius = budget.m_fInfantrySpawnRadius;
			if (profile.m_bVehicleProfile)
				spawnRadius = budget.m_fVehicleSpawnRadius;

			float despawnRadius = Math.Max(spawnRadius, budget.m_fDespawnRadius);
			float spawnRadiusSq = spawnRadius * spawnRadius;
			float despawnRadiusSq = despawnRadius * despawnRadius;

			if (closestDistanceSq <= spawnRadiusSq && profile.m_eState == ARAL_EProfileState.VIRTUAL && activeProfiles < budget.m_iActiveLimiter)
			{
				ARAL_ProfileActivationChange promote = new ARAL_ProfileActivationChange();
				promote.m_sProfileId = profile.m_sId;
				promote.m_eTargetState = ARAL_EProfileState.MATERIALIZED;
				InsertByPriority(warState, promote);
				activeProfiles++;
				continue;
			}

			if (closestDistanceSq > despawnRadiusSq && profile.m_eState == ARAL_EProfileState.MATERIALIZED)
			{
				ARAL_ProfileActivationChange demote = new ARAL_ProfileActivationChange();
				demote.m_sProfileId = profile.m_sId;
				demote.m_eTargetState = ARAL_EProfileState.VIRTUAL;
				m_aQueue.Insert(demote);
				activeProfiles--;
			}
		}
	}

	protected float GetClosestSourceDistanceSq(vector profilePosition, array<vector> sources)
	{
		float closestDistanceSq = 999999999.0;

		if (!sources || sources.IsEmpty())
			return closestDistanceSq;

		foreach (vector source : sources)
		{
			float distanceSq = vector.DistanceSq(profilePosition, source);
			if (distanceSq < closestDistanceSq)
				closestDistanceSq = distanceSq;
		}

		return closestDistanceSq;
	}

	protected void BuildUniqueSpawnSources(array<vector> spawnSources, out array<vector> outSources)
	{
		outSources.Clear();

		if (!spawnSources)
			return;

		float dedupeDistanceSq = SOURCE_DEDUPE_DISTANCE * SOURCE_DEDUPE_DISTANCE;
		foreach (vector source : spawnSources)
		{
			bool isDuplicate = false;
			foreach (vector existingSource : outSources)
			{
				if (vector.DistanceSq(existingSource, source) <= dedupeDistanceSq)
				{
					isDuplicate = true;
					break;
				}
			}

			if (!isDuplicate)
				outSources.Insert(source);
		}
	}

	protected void InsertByPriority(ARAL_WarState warState, ARAL_ProfileActivationChange change)
	{
		ARAL_ForceProfile changeProfile = ARAL_WarStateUtility.FindProfileById(warState, change.m_sProfileId);
		if (!changeProfile)
		{
			m_aQueue.Insert(change);
			return;
		}

		for (int i = 0; i < m_aQueue.Count(); i++)
		{
			ARAL_ForceProfile queuedProfile = ARAL_WarStateUtility.FindProfileById(warState, m_aQueue[i].m_sProfileId);
			if (!queuedProfile)
				continue;

			if (changeProfile.m_iSpawnPriority > queuedProfile.m_iSpawnPriority)
			{
				m_aQueue.InsertAt(change, i);
				return;
			}
		}

		m_aQueue.Insert(change);
	}
}
