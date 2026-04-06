class ARAL_SetupService : Managed
{
	protected ref ARAL_PersistenceService m_PersistenceService;

	void ARAL_SetupService(ARAL_PersistenceService persistenceService)
	{
		m_PersistenceService = persistenceService;
	}

	void GetAvailableFactions(out array<ref ARAL_FactionChoice> outChoices)
	{
		outChoices.Clear();

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
		{
			ARAL_Log.Warning("FactionManager not present, returning no faction choices.");
			return;
		}

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);

		int index = 0;
		foreach (Faction faction : factions)
		{
			if (!faction)
				continue;

			ARAL_FactionChoice choice = new ARAL_FactionChoice();
			choice.m_sKey = string.Format("%1", faction.GetFactionKey());
			choice.m_sName = faction.GetFactionName();
			choice.m_iIndex = index;
			outChoices.Insert(choice);
			index++;
		}
	}

	void ListPresetCatalog(out array<ref ARAL_CatalogEntry> outEntries)
	{
		ListCatalogFromDirectory(ARAL_PathUtility.PRESET_DIR, outEntries);
	}

	void ListSaveCatalog(out array<ref ARAL_CatalogEntry> outEntries)
	{
		ListCatalogFromDirectory(ARAL_PathUtility.SAVE_DIR, outEntries);
	}

	ref ARAL_SetupSnapshot BuildSetupSnapshot(ARAL_MissionPreset draftPreset = null)
	{
		ARAL_SetupSnapshot snapshot = new ARAL_SetupSnapshot();
		if (draftPreset)
			snapshot.m_DraftPreset = draftPreset.Clone();

		GetAvailableFactions(snapshot.m_aAvailableFactions);
		ListPresetCatalog(snapshot.m_aPresetCatalog);
		ListSaveCatalog(snapshot.m_aSaveCatalog);

		NormalizePreset(snapshot.m_DraftPreset, snapshot.m_aAvailableFactions);
		snapshot.m_sResolvedPlayerFactionKey = snapshot.m_DraftPreset.m_sPlayerFactionKey;
		snapshot.m_sResolvedEnemyFactionKey = snapshot.m_DraftPreset.m_sEnemyFactionKey;
		return snapshot;
	}

	void NormalizePreset(ARAL_MissionPreset preset, array<ref ARAL_FactionChoice> availableFactions)
	{
		if (!preset)
			return;

		preset.EnsureDefaults();

		if (!availableFactions || availableFactions.IsEmpty())
			return;

		if (!HasFactionKey(availableFactions, preset.m_sPlayerFactionKey))
			preset.m_sPlayerFactionKey = availableFactions[0].m_sKey;

		if (!HasFactionKey(availableFactions, preset.m_sEnemyFactionKey) || preset.m_sEnemyFactionKey == preset.m_sPlayerFactionKey)
		{
			preset.m_sEnemyFactionKey = availableFactions[0].m_sKey;
			foreach (ARAL_FactionChoice choice : availableFactions)
			{
				if (choice.m_sKey == preset.m_sPlayerFactionKey)
					continue;

				preset.m_sEnemyFactionKey = choice.m_sKey;
				break;
			}
		}
	}

	bool HasSaveForPreset(string presetName)
	{
		array<ref ARAL_CatalogEntry> saves = {};
		ListSaveCatalog(saves);

		foreach (ARAL_CatalogEntry entry : saves)
		{
			if (entry.m_sName == ARAL_PathUtility.SanitiseFileName(presetName) || entry.m_sName == presetName)
				return true;
		}

		return false;
	}

	bool SavePreset(ARAL_MissionPreset preset)
	{
		return m_PersistenceService.SavePreset(preset);
	}

	bool LoadPreset(string presetName, out ARAL_MissionPreset preset)
	{
		return m_PersistenceService.LoadPreset(presetName, preset);
	}

	protected void ListCatalogFromDirectory(string directory, out array<ref ARAL_CatalogEntry> outEntries)
	{
		outEntries.Clear();

		array<ref SCR_FileInfo> files = SCR_FileIOHelper.GetDirectoryContent(directory, ".json");
		if (!files)
			return;

		foreach (SCR_FileInfo fileInfo : files)
		{
			ARAL_CatalogEntry entry = new ARAL_CatalogEntry();
			entry.m_sPath = fileInfo.m_sFilePath;
			entry.m_sName = ARAL_PathUtility.ExtractFileNameWithoutExtension(fileInfo.m_sFilePath);
			outEntries.Insert(entry);
		}
	}

	protected bool HasFactionKey(array<ref ARAL_FactionChoice> availableFactions, string factionKey)
	{
		foreach (ARAL_FactionChoice choice : availableFactions)
		{
			if (choice.m_sKey == factionKey)
				return true;
		}

		return false;
	}
}
