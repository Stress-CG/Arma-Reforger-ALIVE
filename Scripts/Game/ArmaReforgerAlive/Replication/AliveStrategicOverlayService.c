class AliveStrategicOverlaySnapshot : Managed
{
	int m_iGeneration;
	int m_iConnectedPlayerCount;
	int m_iPhysicalProjectionCount;
	int m_iPendingProjectionCount;
	int m_iRawProjectionCount;
	bool m_bAdminOverlayEnabled;
	string m_sObjectiveDigest;
	string m_sTaskDigest;
	string m_sHotProfileDigest;
	string m_sHandoffDigest;
	string m_sProfileOverlayDigest;
	string m_sReclaimOverlayDigest;
}

class AliveStrategicOverlayService : Managed
{
	protected int m_iGeneration;

	AliveStrategicOverlaySnapshot BuildSnapshot(ARAL_WarState warState, int connectedPlayerCount, int physicalProjectionCount, int pendingProjectionCount, int rawProjectionCount, bool adminOverlayEnabled, string handoffDigest, string profileOverlayDigest, string reclaimOverlayDigest)
	{
		AliveStrategicOverlaySnapshot snapshot = new AliveStrategicOverlaySnapshot();
		snapshot.m_iGeneration = ++m_iGeneration;
		snapshot.m_iConnectedPlayerCount = connectedPlayerCount;
		snapshot.m_iPhysicalProjectionCount = physicalProjectionCount;
		snapshot.m_iPendingProjectionCount = pendingProjectionCount;
		snapshot.m_iRawProjectionCount = rawProjectionCount;
		snapshot.m_bAdminOverlayEnabled = adminOverlayEnabled;
		snapshot.m_sObjectiveDigest = BuildObjectiveDigest(warState);
		snapshot.m_sTaskDigest = BuildTaskDigest(warState);
		snapshot.m_sHotProfileDigest = BuildHotProfileDigest(warState);
		snapshot.m_sHandoffDigest = handoffDigest;
		snapshot.m_sProfileOverlayDigest = profileOverlayDigest;
		snapshot.m_sReclaimOverlayDigest = reclaimOverlayDigest;
		return snapshot;
	}

	protected string BuildObjectiveDigest(ARAL_WarState warState)
	{
		if (!warState || warState.m_aObjectives.IsEmpty())
			return "No objectives";

		string digest = "";
		int limit = Math.Min(warState.m_aObjectives.Count(), 5);
		for (int i = 0; i < limit; i++)
		{
			ARAL_Objective objective = warState.m_aObjectives[i];
			string segment = string.Format("%1:%2:%3", objective.m_sName, objective.m_sOwnerFactionKey, objective.m_iPriority);
			if (digest != "")
				digest += " | ";
			digest += segment;
		}

		return digest;
	}

	protected string BuildTaskDigest(ARAL_WarState warState)
	{
		if (!warState || warState.m_aTasks.IsEmpty())
			return "No active tasks";

		string digest = "";
		int limit = Math.Min(warState.m_aTasks.Count(), 4);
		for (int i = 0; i < limit; i++)
		{
			ARAL_TaskRecord task = warState.m_aTasks[i];
			string segment = string.Format("%1:%2", task.m_sTitle, ARAL_EnumUtility.TaskStatusToString(task.m_eStatus));
			if (digest != "")
				digest += " | ";
			digest += segment;
		}

		return digest;
	}

	protected string BuildHotProfileDigest(ARAL_WarState warState)
	{
		if (!warState || warState.m_aProfiles.IsEmpty())
			return "No profiles";

		string digest = "";
		array<string> selectedProfileIds = {};
		int limit = Math.Min(warState.m_aProfiles.Count(), 4);
		for (int i = 0; i < limit; i++)
		{
			ARAL_ForceProfile hotProfile = FindNextBestProfile(warState, selectedProfileIds);
			if (!hotProfile)
				break;

			selectedProfileIds.Insert(hotProfile.m_sId);
			string segment = string.Format("%1:%2:%3", hotProfile.m_sDisplayName, hotProfile.m_sFactionKey, hotProfile.m_iStrength);
			if (digest != "")
				digest += " | ";
			digest += segment;
		}

		return digest;
	}

	protected ARAL_ForceProfile FindNextBestProfile(ARAL_WarState warState, array<string> selectedProfileIds)
	{
		ARAL_ForceProfile bestProfile;
		int bestPriority = -9999;
		int bestStrength = -9999;

		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			if (selectedProfileIds.Find(profile.m_sId) != -1)
				continue;

			if (profile.m_iSpawnPriority > bestPriority)
			{
				bestProfile = profile;
				bestPriority = profile.m_iSpawnPriority;
				bestStrength = profile.m_iStrength;
				continue;
			}

			if (profile.m_iSpawnPriority == bestPriority && profile.m_iStrength > bestStrength)
			{
				bestProfile = profile;
				bestStrength = profile.m_iStrength;
			}
		}

		return bestProfile;
	}
}
