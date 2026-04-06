class ARAL_Log
{
	static void Info(string message)
	{
		Print(string.Format("[ARAL] %1", message), LogLevel.NORMAL);
	}

	static void Warning(string message)
	{
		Print(string.Format("[ARAL] %1", message), LogLevel.WARNING);
	}
}

class ARAL_EnumUtility
{
	static string WarPhaseToString(ARAL_EWarPhase phase)
	{
		switch (phase)
		{
			case ARAL_EWarPhase.UNCONFIGURED:
				return "UNCONFIGURED";
			case ARAL_EWarPhase.SETUP:
				return "SETUP";
			case ARAL_EWarPhase.RUNNING:
				return "RUNNING";
			case ARAL_EWarPhase.PAUSED:
				return "PAUSED";
			case ARAL_EWarPhase.ENDED:
				return "ENDED";
		}

		return "UNKNOWN";
	}

	static string TaskStatusToString(ARAL_ETaskStatus status)
	{
		switch (status)
		{
			case ARAL_ETaskStatus.PENDING:
				return "PENDING";
			case ARAL_ETaskStatus.ACTIVE:
				return "ACTIVE";
			case ARAL_ETaskStatus.COMPLETED:
				return "COMPLETED";
			case ARAL_ETaskStatus.FAILED:
				return "FAILED";
		}

		return "UNKNOWN";
	}

	static string OrderTypeToString(ARAL_EOrderType orderType)
	{
		switch (orderType)
		{
			case ARAL_EOrderType.HOLD:
				return "HOLD";
			case ARAL_EOrderType.ATTACK:
				return "ATTACK";
			case ARAL_EOrderType.DEFEND:
				return "DEFEND";
			case ARAL_EOrderType.REDEPLOY:
				return "REDEPLOY";
			case ARAL_EOrderType.REINFORCE:
				return "REINFORCE";
		}

		return "UNKNOWN";
	}

	static string MaterializationStateToString(ARAL_EMaterializationState state)
	{
		switch (state)
		{
			case ARAL_EMaterializationState.NONE:
				return "NONE";
			case ARAL_EMaterializationState.PENDING:
				return "PENDING";
			case ARAL_EMaterializationState.READY:
				return "READY";
			case ARAL_EMaterializationState.VIRTUALIZED:
				return "VIRTUALIZED";
		}

		return "UNKNOWN";
	}

	static string HandoffStateToString(AliveEHandoffState state)
	{
		switch (state)
		{
			case AliveEHandoffState.Virtual:
				return "Virtual";
			case AliveEHandoffState.PendingSpawn:
				return "PendingSpawn";
			case AliveEHandoffState.Spawning:
				return "Spawning";
			case AliveEHandoffState.Physical:
				return "Physical";
			case AliveEHandoffState.PendingReclaim:
				return "PendingReclaim";
			case AliveEHandoffState.Reclaiming:
				return "Reclaiming";
			case AliveEHandoffState.Destroyed:
				return "Destroyed";
		}

		return "Unknown";
	}

	static string TacticalModeToString(AliveETacticalMode mode)
	{
		switch (mode)
		{
			case AliveETacticalMode.None:
				return "None";
			case AliveETacticalMode.Hold:
				return "Hold";
			case AliveETacticalMode.Move:
				return "Move";
			case AliveETacticalMode.Defend:
				return "Defend";
			case AliveETacticalMode.Patrol:
				return "Patrol";
			case AliveETacticalMode.LimitedAssault:
				return "LimitedAssault";
			case AliveETacticalMode.MountedMovement:
				return "MountedMovement";
			case AliveETacticalMode.CombatPatrol:
				return "CombatPatrol";
			case AliveETacticalMode.Transport:
				return "Transport";
		}

		return "Unknown";
	}
}

class ARAL_IdUtility
{
	static string MakeStableId(string prefix, string name, vector position, int ordinal = 0)
	{
		int posX = Math.Round(position[0]);
		int posZ = Math.Round(position[2]);
		return string.Format("%1_%2_%3_%4_%5", prefix, ordinal, posX, posZ, name);
	}
}

class ARAL_PathUtility
{
	static const string ROOT_DIR = "$profile:/ReforgerAlive";
	static const string PRESET_DIR = "$profile:/ReforgerAlive/Presets";
	static const string SAVE_DIR = "$profile:/ReforgerAlive/Saves";
	static const string REPORT_DIR = "$profile:/ReforgerAlive/Reports";

	static string SanitiseFileName(string name)
	{
		string sanitised = SCR_FileIOHelper.SanitiseFileName(name);
		if (sanitised == "")
			sanitised = "Campaign";

		return sanitised;
	}

	static string BuildPresetPath(string presetName)
	{
		return string.Format("%1/%2.json", PRESET_DIR, SanitiseFileName(presetName));
	}

	static string BuildSavePath(string presetName)
	{
		return string.Format("%1/%2.json", SAVE_DIR, SanitiseFileName(presetName));
	}

	static string BuildReportPath(string presetName)
	{
		return string.Format("%1/%2-status.txt", REPORT_DIR, SanitiseFileName(presetName));
	}

	static string ExtractFileNameWithoutExtension(string filePath)
	{
		string normalised = filePath;
		normalised.Replace("\\", "/");

		array<string> pathParts = {};
		normalised.Split("/", pathParts, true);

		if (pathParts.IsEmpty())
			return filePath;

		string fileName = pathParts[pathParts.Count() - 1];
		fileName.Replace(".json", "");
		fileName.Replace(".txt", "");
		return fileName;
	}
}

class ARAL_WarStateUtility
{
	static ARAL_Objective FindObjectiveById(ARAL_WarState warState, string objectiveId)
	{
		if (!warState)
			return null;

		foreach (ARAL_Objective objective : warState.m_aObjectives)
		{
			if (objective.m_sId == objectiveId)
				return objective;
		}

		return null;
	}

	static ARAL_ForceProfile FindProfileById(ARAL_WarState warState, string profileId)
	{
		if (!warState)
			return null;

		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			if (profile.m_sId == profileId)
				return profile;
		}

		return null;
	}

	static ARAL_Order FindOrderById(ARAL_WarState warState, string orderId)
	{
		if (!warState)
			return null;

		foreach (ARAL_Order order : warState.m_aOrders)
		{
			if (order.m_sId == orderId)
				return order;
		}

		return null;
	}

	static ARAL_MaterializationRequest FindMaterializationRequestByProfileId(ARAL_WarState warState, string profileId)
	{
		if (!warState)
			return null;

		foreach (ARAL_MaterializationRequest request : warState.m_aMaterializationRequests)
		{
			if (request.m_sProfileId == profileId)
				return request;
		}

		return null;
	}

	static int CountProfilesInState(ARAL_WarState warState, ARAL_EProfileState state)
	{
		if (!warState)
			return 0;

		int count = 0;

		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			if (profile.m_eState == state)
				count++;
		}

		return count;
	}

	static int CountMaterializationRequestsInState(ARAL_WarState warState, ARAL_EMaterializationState state)
	{
		if (!warState)
			return 0;

		int count = 0;
		foreach (ARAL_MaterializationRequest request : warState.m_aMaterializationRequests)
		{
			if (request.m_eState == state)
				count++;
		}

		return count;
	}
}
