class ARAL_PersistenceBackendBase : Managed
{
	bool SavePreset(ARAL_MissionPreset preset)
	{
		return false;
	}

	bool LoadPreset(string presetName, out ARAL_MissionPreset preset)
	{
		return false;
	}

	bool SaveEnvelope(ARAL_PersistenceEnvelope envelope)
	{
		return false;
	}

	bool LoadEnvelope(string presetName, out ARAL_PersistenceEnvelope envelope)
	{
		return false;
	}
}

class ARAL_LocalPersistenceBackend : ARAL_PersistenceBackendBase
{
	protected void EnsureDirectories()
	{
		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.ROOT_DIR);
		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.PRESET_DIR);
		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.SAVE_DIR);
	}

	override bool SavePreset(ARAL_MissionPreset preset)
	{
		EnsureDirectories();
		preset.EnsureDefaults();

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		saveContext.WriteValue("", preset);
		return saveContext.SaveToFile(ARAL_PathUtility.BuildPresetPath(preset.m_sName));
	}

	override bool LoadPreset(string presetName, out ARAL_MissionPreset preset)
	{
		EnsureDirectories();

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.LoadFromFile(ARAL_PathUtility.BuildPresetPath(presetName)))
			return false;

		preset = new ARAL_MissionPreset();
		if (!loadContext.ReadValue("", preset))
			return false;

		preset.EnsureDefaults();
		return true;
	}

	override bool SaveEnvelope(ARAL_PersistenceEnvelope envelope)
	{
		EnsureDirectories();
		envelope.EnsureDefaults();

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		saveContext.WriteValue("", envelope);
		return saveContext.SaveToFile(ARAL_PathUtility.BuildSavePath(envelope.m_Preset.m_sName));
	}

	override bool LoadEnvelope(string presetName, out ARAL_PersistenceEnvelope envelope)
	{
		EnsureDirectories();

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.LoadFromFile(ARAL_PathUtility.BuildSavePath(presetName)))
			return false;

		envelope = new ARAL_PersistenceEnvelope();
		if (!loadContext.ReadValue("", envelope))
			return false;

		envelope.EnsureDefaults();
		return true;
	}
}

class ARAL_PersistenceService : Managed
{
	protected ref ARAL_PersistenceBackendBase m_Backend;

	void ARAL_PersistenceService(ARAL_PersistenceBackendBase backend = null)
	{
		if (backend)
			m_Backend = backend;
		else
			m_Backend = new ARAL_LocalPersistenceBackend();
	}

	ARAL_PersistenceBackendBase GetBackend()
	{
		return m_Backend;
	}

	bool SavePreset(ARAL_MissionPreset preset)
	{
		if (!preset)
			return false;

		return m_Backend.SavePreset(preset);
	}

	bool LoadPreset(string presetName, out ARAL_MissionPreset preset)
	{
		return m_Backend.LoadPreset(presetName, preset);
	}

	bool SaveWarState(ARAL_MissionPreset preset, ARAL_WarState warState)
	{
		if (!preset || !warState)
			return false;

		ARAL_PersistenceEnvelope envelope = new ARAL_PersistenceEnvelope();
		envelope.m_fSavedAt = System.GetUnixTime();
		envelope.m_Preset = preset.Clone();
		envelope.m_WarState = BuildPersistenceWarStateSnapshot(warState);
		return m_Backend.SaveEnvelope(envelope);
	}

	bool LoadWarState(string presetName, out ARAL_MissionPreset preset, out ARAL_WarState warState)
	{
		ARAL_PersistenceEnvelope envelope;
		if (!m_Backend.LoadEnvelope(presetName, envelope))
			return false;

		preset = envelope.m_Preset;
		warState = envelope.m_WarState;
		warState.EnsureCollections();
		warState.m_aMaterializationRequests.Clear();
		return true;
	}

	protected ARAL_WarState BuildPersistenceWarStateSnapshot(ARAL_WarState warState)
	{
		ARAL_WarState snapshot = warState.Clone();
		snapshot.EnsureCollections();
		// Materialization requests are derived runtime state and should not be persisted in V1.
		snapshot.m_aMaterializationRequests.Clear();
		return snapshot;
	}
}
