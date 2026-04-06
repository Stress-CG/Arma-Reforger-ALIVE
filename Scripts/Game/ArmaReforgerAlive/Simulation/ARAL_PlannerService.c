class ARAL_PlannerService : Managed
{
	protected float m_fPlanAccumulator;
	protected static const float PLAN_PERIOD_SECONDS = 10.0;
	protected ref ARAL_FactionTemplateService m_FactionTemplateService = new ARAL_FactionTemplateService();

	void InitializeForcePools(ARAL_MissionPreset preset, ARAL_WarState warState)
	{
		if (!warState || warState.m_aObjectives.IsEmpty())
			return;

		warState.m_aProfiles.Clear();
		warState.m_aMaterializationRequests.Clear();
		warState.m_aOrders.Clear();
		warState.m_aTasks.Clear();

		ARAL_Objective playerAnchor = warState.m_aObjectives[0];
		ARAL_Objective enemyAnchor = warState.m_aObjectives[warState.m_aObjectives.Count() - 1];

		for (int i = 0; i < preset.m_iInitialProfilesPerSide; i++)
		{
			ARAL_ForceProfile playerProfile = BuildProfile(preset.m_sPlayerFactionKey, playerAnchor, i);
			ARAL_ForceProfile enemyProfile = BuildProfile(preset.m_sEnemyFactionKey, enemyAnchor, i);
			warState.m_aProfiles.Insert(playerProfile);
			warState.m_aProfiles.Insert(enemyProfile);
		}

		SeedObjectiveOwnership(warState, playerAnchor, enemyAnchor, preset);
		AssignOrders(warState, 0);
		RefreshTasks(warState);
	}

	void Tick(ARAL_WarState warState, float timeSlice)
	{
		if (!warState || warState.m_ePhase != ARAL_EWarPhase.RUNNING)
			return;

		m_fPlanAccumulator += timeSlice;
		if (m_fPlanAccumulator < PLAN_PERIOD_SECONDS)
			return;

		m_fPlanAccumulator = 0;
		ResolveVirtualMovement(warState);
		ResolveObjectiveOwnership(warState);
		ResolveVirtualCombat(warState);
		AssignOrders(warState, System.GetUnixTime());
		RefreshTasks(warState);
	}

	protected ARAL_ForceProfile BuildProfile(string factionKey, ARAL_Objective anchor, int ordinal)
	{
		ARAL_ProfileTemplate template = m_FactionTemplateService.GetTemplateForIndex(factionKey, ordinal);
		ARAL_ForceProfile profile = new ARAL_ForceProfile();
		profile.m_sFactionKey = factionKey;
		profile.m_sTemplateId = template.m_sId;
		profile.m_sDisplayName = template.m_sDisplayName;
		profile.m_sAnchorObjectiveId = anchor.m_sId;
		profile.m_vVirtualPosition = anchor.m_vPosition;
		profile.m_iStrength = template.m_iStrength;
		profile.m_iSpawnPriority = template.m_iSpawnPriority;
		profile.m_bVehicleProfile = template.m_bVehicleProfile;
		profile.m_sId = ARAL_IdUtility.MakeStableId("profile", factionKey, anchor.m_vPosition, ordinal);
		return profile;
	}

	protected void AssignOrders(ARAL_WarState warState, float issuedAt)
	{
		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			ARAL_Objective targetObjective = SelectObjectiveForProfile(warState, profile);
			if (!targetObjective)
				continue;

			ARAL_Order order = FindOrCreateOrder(warState, profile);
			order.m_sObjectiveId = targetObjective.m_sId;
			order.m_eType = targetObjective.m_sOwnerFactionKey == profile.m_sFactionKey ? ARAL_EOrderType.DEFEND : ARAL_EOrderType.ATTACK;
			order.m_iPriority = targetObjective.m_iPriority;
			order.m_fIssuedAt = issuedAt;
			order.m_fDueAt = issuedAt + 300;
			profile.m_sActiveOrderId = order.m_sId;
		}
	}

	protected ARAL_Order FindOrCreateOrder(ARAL_WarState warState, ARAL_ForceProfile profile)
	{
		ARAL_Order existing = ARAL_WarStateUtility.FindOrderById(warState, profile.m_sActiveOrderId);
		if (existing)
			return existing;

		ARAL_Order order = new ARAL_Order();
		order.m_sProfileId = profile.m_sId;
		order.m_sId = ARAL_IdUtility.MakeStableId("order", profile.m_sFactionKey, profile.m_vVirtualPosition, warState.m_aOrders.Count());
		warState.m_aOrders.Insert(order);
		return order;
	}

	protected ARAL_Objective SelectObjectiveForProfile(ARAL_WarState warState, ARAL_ForceProfile profile)
	{
		ARAL_Objective bestObjective;
		float bestScore = -1;

		foreach (ARAL_Objective objective : warState.m_aObjectives)
		{
			float distance = vector.Distance(profile.m_vVirtualPosition, objective.m_vPosition);
			float distanceScore = 1.0 / Math.Max(distance, 1);
			float ownerPenalty = 0;
			if (objective.m_sOwnerFactionKey == profile.m_sFactionKey)
				ownerPenalty = 0.15;

			float score = objective.m_iPriority + (distanceScore * 100) - ownerPenalty;
			if (score > bestScore)
			{
				bestObjective = objective;
				bestScore = score;
			}
		}

		return bestObjective;
	}

	protected void ResolveVirtualMovement(ARAL_WarState warState)
	{
		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			if (profile.m_eState == ARAL_EProfileState.MATERIALIZED)
				continue;

			ARAL_Order order = ARAL_WarStateUtility.FindOrderById(warState, profile.m_sActiveOrderId);
			if (!order)
				continue;

			ARAL_Objective objective = ARAL_WarStateUtility.FindObjectiveById(warState, order.m_sObjectiveId);
			if (!objective)
				continue;

			vector delta = objective.m_vPosition - profile.m_vVirtualPosition;
			float distance = delta.Length();
			if (distance <= 1)
				continue;

			vector direction = delta / distance;
			float movementStep = 25;
			if (profile.m_bVehicleProfile)
				movementStep = 45;

			profile.m_vVirtualPosition = profile.m_vVirtualPosition + (direction * movementStep);
		}
	}

	protected void ResolveObjectiveOwnership(ARAL_WarState warState)
	{
		if (warState.m_aProfiles.Count() < 2)
			return;

		string playerFaction = warState.m_aProfiles[0].m_sFactionKey;
		string enemyFaction = warState.m_aProfiles[1].m_sFactionKey;

		foreach (ARAL_Objective objective : warState.m_aObjectives)
		{
			int playerStrength = 0;
			int enemyStrength = 0;

			foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
			{
				if (vector.Distance(profile.m_vVirtualPosition, objective.m_vPosition) > objective.m_fRadius)
					continue;

				if (profile.m_sFactionKey == playerFaction)
					playerStrength += profile.m_iStrength;
				else if (profile.m_sFactionKey == enemyFaction)
					enemyStrength += profile.m_iStrength;
			}

			if (playerStrength > enemyStrength && playerStrength > 0)
				objective.m_sOwnerFactionKey = playerFaction;
			else if (enemyStrength > playerStrength && enemyStrength > 0)
				objective.m_sOwnerFactionKey = enemyFaction;
		}
	}

	protected void SeedObjectiveOwnership(ARAL_WarState warState, ARAL_Objective playerAnchor, ARAL_Objective enemyAnchor, ARAL_MissionPreset preset)
	{
		foreach (ARAL_Objective objective : warState.m_aObjectives)
		{
			if (objective.m_sId == playerAnchor.m_sId)
			{
				objective.m_sOwnerFactionKey = preset.m_sPlayerFactionKey;
				continue;
			}

			if (objective.m_sId == enemyAnchor.m_sId)
			{
				objective.m_sOwnerFactionKey = preset.m_sEnemyFactionKey;
				continue;
			}

			float distanceToPlayer = vector.Distance(objective.m_vPosition, playerAnchor.m_vPosition);
			float distanceToEnemy = vector.Distance(objective.m_vPosition, enemyAnchor.m_vPosition);
			if (distanceToPlayer <= distanceToEnemy)
				objective.m_sOwnerFactionKey = preset.m_sPlayerFactionKey;
			else
				objective.m_sOwnerFactionKey = preset.m_sEnemyFactionKey;
		}
	}

	protected void ResolveVirtualCombat(ARAL_WarState warState)
	{
		for (int i = 0; i < warState.m_aProfiles.Count(); i++)
		{
			ARAL_ForceProfile left = warState.m_aProfiles[i];
			if (left.m_eState == ARAL_EProfileState.MATERIALIZED)
				continue;

			for (int j = i + 1; j < warState.m_aProfiles.Count(); j++)
			{
				ARAL_ForceProfile right = warState.m_aProfiles[j];
				if (left.m_sFactionKey == right.m_sFactionKey)
					continue;

				if (right.m_eState == ARAL_EProfileState.MATERIALIZED)
					continue;

				if (vector.Distance(left.m_vVirtualPosition, right.m_vVirtualPosition) > 250)
					continue;

				left.m_iStrength = Math.Max(left.m_iStrength - 1, 1);
				right.m_iStrength = Math.Max(right.m_iStrength - 1, 1);
			}
		}
	}

	protected void RefreshTasks(ARAL_WarState warState)
	{
		warState.m_aTasks.Clear();
		if (warState.m_aProfiles.Count() < 2)
			return;

		string playerFaction = warState.m_aProfiles[0].m_sFactionKey;

		int created = 0;
		foreach (ARAL_Objective objective : warState.m_aObjectives)
		{
			if (objective.m_sOwnerFactionKey == playerFaction)
				continue;

			ARAL_TaskRecord task = new ARAL_TaskRecord();
			task.m_sId = ARAL_IdUtility.MakeStableId("task", objective.m_sName, objective.m_vPosition, created);
			task.m_sTitle = string.Format("Secure %1", objective.m_sName);
			task.m_sDetails = "ALIVE planner-generated operational task.";
			task.m_sObjectiveId = objective.m_sId;
			task.m_sFactionKey = objective.m_sOwnerFactionKey;
			task.m_eStatus = ARAL_ETaskStatus.ACTIVE;
			warState.m_aTasks.Insert(task);
			created++;

			if (created >= 5)
				break;
		}

		if (created > 0)
			return;

		int limit = Math.Min(warState.m_aObjectives.Count(), 3);
		for (int i = 0; i < limit; i++)
		{
			ARAL_Objective fallbackObjective = warState.m_aObjectives[i];
			ARAL_TaskRecord fallbackTask = new ARAL_TaskRecord();
			fallbackTask.m_sId = ARAL_IdUtility.MakeStableId("task", fallbackObjective.m_sName, fallbackObjective.m_vPosition, i);
			fallbackTask.m_sTitle = string.Format("Hold %1", fallbackObjective.m_sName);
			fallbackTask.m_sDetails = "No hostile objectives found; maintain control of owned ground.";
			fallbackTask.m_sObjectiveId = fallbackObjective.m_sId;
			fallbackTask.m_sFactionKey = fallbackObjective.m_sOwnerFactionKey;
			fallbackTask.m_eStatus = ARAL_ETaskStatus.ACTIVE;
			warState.m_aTasks.Insert(fallbackTask);
		}
	}
}

class ARAL_SimulationService : Managed
{
	protected ref ARAL_PlannerService m_PlannerService = new ARAL_PlannerService();
	protected ref ARAL_ActivationService m_ActivationService = new ARAL_ActivationService();
	protected ref ARAL_MaterializationService m_MaterializationService = new ARAL_MaterializationService();
	protected ref AliveHandoffManager m_HandoffManager = new AliveHandoffManager();

	void InitializeWar(ARAL_MissionPreset preset, ARAL_WarState warState)
	{
		m_HandoffManager.ResetRuntime(true);
		m_PlannerService.InitializeForcePools(preset, warState);
	}

	void ResetRuntime(bool deletePhysicalProjections = true)
	{
		m_HandoffManager.ResetRuntime(deletePhysicalProjections);
	}

	void Tick(IEntity owner, ARAL_WarState warState, ARAL_MissionPreset preset, float timeSlice, array<vector> spawnSources)
	{
		m_PlannerService.Tick(warState, timeSlice);
		m_ActivationService.Step(warState, preset.m_ActivationBudget, timeSlice, spawnSources);
		m_HandoffManager.Tick(owner, warState, preset, spawnSources);
		m_MaterializationService.ReconcileRequests(warState, m_HandoffManager);
	}

	int GetPhysicalProjectionCount()
	{
		return m_HandoffManager.GetPhysicalProjectionCount();
	}

	int GetPendingProjectionCount()
	{
		return m_HandoffManager.GetPendingProjectionCount();
	}

	int GetRawProjectionCount()
	{
		return m_HandoffManager.GetRawProjectionCount();
	}

	string BuildHandoffDebugSummary(ARAL_WarState warState)
	{
		return m_HandoffManager.BuildDebugSummary(warState);
	}

	string BuildAdminProfileOverlay(ARAL_WarState warState)
	{
		return m_HandoffManager.BuildAdminProfileOverlay(warState);
	}

	string BuildAdminReclaimOverlay(ARAL_WarState warState)
	{
		return m_HandoffManager.BuildAdminReclaimOverlay(warState);
	}

	ARAL_WarState BuildPersistenceSnapshot(ARAL_WarState warState)
	{
		return m_HandoffManager.BuildPersistenceSnapshot(warState);
	}
}
