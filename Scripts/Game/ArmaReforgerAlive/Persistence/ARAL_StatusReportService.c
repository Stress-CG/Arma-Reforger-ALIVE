class ARAL_StatusReportService : Managed
{
	bool WriteReport(ARAL_MissionPreset preset, ARAL_WarState warState, int connectedPlayerCount = 0, int physicalProjectionCount = 0, int pendingProjectionCount = 0, int rawProjectionCount = 0, string handoffSummary = "", string profileOverlay = "", string reclaimOverlay = "")
	{
		if (!preset || !warState)
			return false;

		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.ROOT_DIR);
		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.REPORT_DIR);

		array<string> lines = {};
		lines.Insert(string.Format("Preset: %1", preset.m_sName));
		lines.Insert(string.Format("Description: %1", preset.m_sDescription));
		lines.Insert(string.Format("Policy: %1", ARAL_ProjectPolicy.BuildPolicySummary()));
		lines.Insert(string.Format("Phase: %1", ARAL_EnumUtility.WarPhaseToString(warState.m_ePhase)));
		lines.Insert(string.Format("Player Faction: %1", preset.m_sPlayerFactionKey));
		lines.Insert(string.Format("Enemy Faction: %1", preset.m_sEnemyFactionKey));
		lines.Insert(string.Format("Connected Players: %1", connectedPlayerCount));
		lines.Insert(string.Format("Objectives: %1", warState.m_aObjectives.Count()));
		lines.Insert(string.Format("Profiles: %1", warState.m_aProfiles.Count()));
		lines.Insert(string.Format("Materialized Profiles: %1", ARAL_WarStateUtility.CountProfilesInState(warState, ARAL_EProfileState.MATERIALIZED)));
		lines.Insert(string.Format("Physical Projections: %1", physicalProjectionCount));
		lines.Insert(string.Format("Raw Projection Fallbacks: %1", rawProjectionCount));
		lines.Insert(string.Format("Pending Handoffs: %1", pendingProjectionCount));
		lines.Insert(string.Format("Ready Materialization Requests: %1", ARAL_WarStateUtility.CountMaterializationRequestsInState(warState, ARAL_EMaterializationState.READY)));
		lines.Insert(string.Format("Orders: %1", warState.m_aOrders.Count()));
		lines.Insert(string.Format("Tasks: %1", warState.m_aTasks.Count()));
		if (handoffSummary != "")
			lines.Insert(string.Format("Handoff Summary: %1", handoffSummary));
		if (profileOverlay != "")
			lines.Insert(string.Format("Admin Profiles: %1", profileOverlay));
		if (reclaimOverlay != "")
			lines.Insert(string.Format("Admin Reclaim: %1", reclaimOverlay));
		lines.Insert("");
		lines.Insert("Top Tasks:");

		int taskLimit = Math.Min(warState.m_aTasks.Count(), 5);
		for (int i = 0; i < taskLimit; i++)
		{
			ARAL_TaskRecord task = warState.m_aTasks[i];
			lines.Insert(string.Format("- %1 [%2]", task.m_sTitle, ARAL_EnumUtility.TaskStatusToString(task.m_eStatus)));
		}

		lines.Insert("");
		lines.Insert("Key Objectives:");

		int objectiveLimit = Math.Min(warState.m_aObjectives.Count(), 8);
		for (int j = 0; j < objectiveLimit; j++)
		{
			ARAL_Objective objective = warState.m_aObjectives[j];
			lines.Insert(string.Format("- %1 | owner=%2 | priority=%3 | pos=%4", objective.m_sName, objective.m_sOwnerFactionKey, objective.m_iPriority, objective.m_vPosition));
		}

		lines.Insert("");
		lines.Insert("Materialization Queue:");
		int requestLimit = Math.Min(warState.m_aMaterializationRequests.Count(), 8);
		for (int k = 0; k < requestLimit; k++)
		{
			ARAL_MaterializationRequest request = warState.m_aMaterializationRequests[k];
			lines.Insert(string.Format("- %1 | faction=%2 | template=%3 | state=%4 | count=%5", request.m_sDisplayName, request.m_sFactionKey, request.m_sTemplateId, ARAL_EnumUtility.MaterializationStateToString(request.m_eState), request.m_iDesiredCount));
		}

		return SCR_FileIOHelper.WriteFileContent(ARAL_PathUtility.BuildReportPath(preset.m_sName), lines);
	}
}
