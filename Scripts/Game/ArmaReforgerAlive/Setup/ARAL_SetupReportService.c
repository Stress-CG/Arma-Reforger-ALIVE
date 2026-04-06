class ARAL_SetupReportService : Managed
{
	bool WriteSnapshotReport(ARAL_SetupSnapshot snapshot)
	{
		if (!snapshot)
			return false;

		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.ROOT_DIR);
		SCR_FileIOHelper.CreateDirectory(ARAL_PathUtility.REPORT_DIR);

		string presetName = "SetupSnapshot";
		if (snapshot.m_DraftPreset)
			presetName = snapshot.m_DraftPreset.m_sName + "-setup";

		array<string> lines = {};
		lines.Insert("Reforger ALIVE Setup Snapshot");
		lines.Insert("");

		if (snapshot.m_DraftPreset)
		{
			lines.Insert(string.Format("Preset: %1", snapshot.m_DraftPreset.m_sName));
			lines.Insert(string.Format("Description: %1", snapshot.m_DraftPreset.m_sDescription));
			lines.Insert(string.Format("Player Faction: %1", snapshot.m_sResolvedPlayerFactionKey));
			lines.Insert(string.Format("Enemy Faction: %1", snapshot.m_sResolvedEnemyFactionKey));
			lines.Insert(string.Format("Objective Limit: %1", snapshot.m_DraftPreset.m_iObjectiveLimit));
			lines.Insert(string.Format("Profiles Per Side: %1", snapshot.m_DraftPreset.m_iInitialProfilesPerSide));
			lines.Insert(string.Format("Scan Radius: %1", snapshot.m_DraftPreset.m_fWorldScanRadius));
			lines.Insert(string.Format("Auto Scan: %1", snapshot.m_DraftPreset.m_bAutoScanMap));
			lines.Insert(string.Format("Use Overrides: %1", snapshot.m_DraftPreset.m_bUseAuthorOverrides));
			lines.Insert("");
		}

		lines.Insert("Available Factions:");
		foreach (ARAL_FactionChoice choice : snapshot.m_aAvailableFactions)
			lines.Insert(string.Format("- %1 (%2)", choice.m_sName, choice.m_sKey));

		lines.Insert("");
		lines.Insert("Preset Catalog:");
		if (snapshot.m_aPresetCatalog.IsEmpty())
			lines.Insert("- none");
		else
		{
			foreach (ARAL_CatalogEntry presetEntry : snapshot.m_aPresetCatalog)
				lines.Insert(string.Format("- %1", presetEntry.m_sName));
		}

		lines.Insert("");
		lines.Insert("Save Catalog:");
		if (snapshot.m_aSaveCatalog.IsEmpty())
			lines.Insert("- none");
		else
		{
			foreach (ARAL_CatalogEntry saveEntry : snapshot.m_aSaveCatalog)
				lines.Insert(string.Format("- %1", saveEntry.m_sName));
		}

		return SCR_FileIOHelper.WriteFileContent(ARAL_PathUtility.BuildReportPath(presetName), lines);
	}
}
