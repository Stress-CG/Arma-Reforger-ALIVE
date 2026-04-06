[ComponentEditorProps(category: "Arma Reforger Alive/Core", description: "Bootstraps the Reforger ALIVE runtime on a world entity.", color: "0.8 0.2 0.2 1", visible: true)]
class ARAL_BootstrapComponentClass : ScriptComponentClass
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};
		requires.Insert(ARAL_RuntimeStateComponent);
		requires.Insert(AliveSpawnCatalogComponent);
		requires.Insert(AliveStrategicOverlayComponent);
		return requires;
	}
}

class ARAL_BootstrapComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Start the campaign automatically on the authority machine.", category: "Runtime")]
	protected bool m_bAutoStartOnAuthority;

	[Attribute("1", UIWidgets.CheckBox, "Try to load an existing save with the preset name before starting a fresh campaign.", category: "Runtime")]
	protected bool m_bAutoLoadSaveBeforeStart;

	[Attribute("0", UIWidgets.CheckBox, "If enabled, the campaign begins paused after bootstrapping.", category: "Runtime")]
	protected bool m_bStartPaused;

	[Attribute("1", UIWidgets.CheckBox, "Write a text status report each time the framework saves war state.", category: "Runtime")]
	protected bool m_bWriteStatusReports;

	[Attribute("1", UIWidgets.CheckBox, "Enable GM/admin strategic overlay replication payloads.", category: "Runtime")]
	protected bool m_bAdminOverlayEnabled;

	[Attribute("ALIVE Framework Demo", UIWidgets.EditBox, "Preset name to save and reuse.", category: "Preset")]
	protected string m_sPresetName;

	[Attribute("Runtime-configurable reusable framework preset.", UIWidgets.EditBox, "Preset description.", category: "Preset")]
	protected string m_sPresetDescription;

	[Attribute("US", UIWidgets.EditBox, "Player-side faction key.", category: "Preset")]
	protected string m_sPlayerFactionKey;

	[Attribute("USSR", UIWidgets.EditBox, "Enemy faction key.", category: "Preset")]
	protected string m_sEnemyFactionKey;

	[Attribute("1", UIWidgets.CheckBox, "Discover objectives from map data when possible.", category: "World")]
	protected bool m_bAutoScanMap;

	[Attribute("1", UIWidgets.CheckBox, "Accept author-placed objective seed components.", category: "World")]
	protected bool m_bUseAuthorOverrides;

	[Attribute("20000", UIWidgets.EditBox, "World scan radius in meters.", params: "500 100000 100", category: "World")]
	protected float m_fWorldScanRadius;

	[Attribute("48", UIWidgets.EditBox, "Maximum objectives to keep from scanning.", params: "1 256 1", category: "World")]
	protected int m_iObjectiveLimit;

	[Attribute("12", UIWidgets.EditBox, "Initial profiles created per faction.", params: "1 128 1", category: "Simulation")]
	protected int m_iProfilesPerSide;

	[Attribute("1200", UIWidgets.EditBox, "Spawn radius for infantry profiles.", params: "50 5000 50", category: "Simulation")]
	protected float m_fInfantrySpawnRadius;

	[Attribute("1800", UIWidgets.EditBox, "Spawn radius for vehicle profiles.", params: "50 10000 50", category: "Simulation")]
	protected float m_fVehicleSpawnRadius;

	[Attribute("2200", UIWidgets.EditBox, "Despawn radius for active profiles.", params: "50 10000 50", category: "Simulation")]
	protected float m_fDespawnRadius;

	[Attribute("0.3", UIWidgets.EditBox, "Seconds between activation queue dispatches.", params: "0.05 5 0.05", category: "Simulation")]
	protected float m_fSmoothSpawnSeconds;

	[Attribute("72", UIWidgets.EditBox, "Maximum concurrently materialized profiles.", params: "1 512 1", category: "Simulation")]
	protected int m_iActiveLimiter;

	[Attribute("120", UIWidgets.EditBox, "Persistence save interval in seconds.", params: "5 3600 5", category: "Persistence")]
	protected float m_fSaveInterval;

	protected ARAL_RuntimeStateComponent m_RuntimeState;
	protected bool m_bAttemptedAutostart;

	override void OnPostInit(IEntity owner)
	{
		m_RuntimeState = ARAL_RuntimeStateComponent.Cast(owner.FindComponent(ARAL_RuntimeStateComponent));
		ARAL_Runtime.GetInstance().Initialize(owner, m_RuntimeState);
		ARAL_Runtime.GetInstance().SetWriteStatusReports(m_bWriteStatusReports);
		ARAL_Runtime.GetInstance().SetAdminOverlayEnabled(m_bAdminOverlayEnabled);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (m_bAutoStartOnAuthority && !m_bAttemptedAutostart)
		{
			m_bAttemptedAutostart = true;
			bool started = ARAL_Runtime.GetInstance().LoadOrStartCampaign(BuildPreset(), m_bAutoLoadSaveBeforeStart);
			if (started && m_bStartPaused)
				ARAL_Runtime.GetInstance().PauseCampaign();
		}

		ARAL_Runtime.GetInstance().Tick(timeSlice);
	}

	ARAL_MissionPreset BuildPreset()
	{
		ARAL_MissionPreset preset = new ARAL_MissionPreset();
		preset.m_sName = m_sPresetName;
		preset.m_sDescription = m_sPresetDescription;
		preset.m_sPlayerFactionKey = m_sPlayerFactionKey;
		preset.m_sEnemyFactionKey = m_sEnemyFactionKey;
		preset.m_bAutoScanMap = m_bAutoScanMap;
		preset.m_bUseAuthorOverrides = m_bUseAuthorOverrides;
		preset.m_fWorldScanRadius = m_fWorldScanRadius;
		preset.m_iObjectiveLimit = m_iObjectiveLimit;
		preset.m_iInitialProfilesPerSide = m_iProfilesPerSide;
		preset.m_fSaveIntervalSeconds = m_fSaveInterval;
		preset.m_ActivationBudget.m_fInfantrySpawnRadius = m_fInfantrySpawnRadius;
		preset.m_ActivationBudget.m_fVehicleSpawnRadius = m_fVehicleSpawnRadius;
		preset.m_ActivationBudget.m_fDespawnRadius = m_fDespawnRadius;
		preset.m_ActivationBudget.m_fSmoothSpawnSeconds = m_fSmoothSpawnSeconds;
		preset.m_ActivationBudget.m_iActiveLimiter = m_iActiveLimiter;
		return preset;
	}

	string GetPresetName()
	{
		return m_sPresetName;
	}

	string GetPresetDescription()
	{
		return m_sPresetDescription;
	}

	string GetPlayerFactionKey()
	{
		return m_sPlayerFactionKey;
	}

	string GetEnemyFactionKey()
	{
		return m_sEnemyFactionKey;
	}

	bool GetAutoScanMap()
	{
		return m_bAutoScanMap;
	}

	bool GetUseAuthorOverrides()
	{
		return m_bUseAuthorOverrides;
	}

	float GetWorldScanRadius()
	{
		return m_fWorldScanRadius;
	}

	int GetObjectiveLimit()
	{
		return m_iObjectiveLimit;
	}

	int GetProfilesPerSide()
	{
		return m_iProfilesPerSide;
	}

	float GetInfantrySpawnRadius()
	{
		return m_fInfantrySpawnRadius;
	}

	float GetVehicleSpawnRadius()
	{
		return m_fVehicleSpawnRadius;
	}

	float GetDespawnRadius()
	{
		return m_fDespawnRadius;
	}

	float GetSmoothSpawnSeconds()
	{
		return m_fSmoothSpawnSeconds;
	}

	int GetActiveLimiter()
	{
		return m_iActiveLimiter;
	}

	float GetSaveInterval()
	{
		return m_fSaveInterval;
	}

	[ButtonAttribute("Toggle Admin Overlay")]
	void EditorToggleAdminOverlay()
	{
		if (!Replication.IsServer())
			return;

		m_bAdminOverlayEnabled = !m_bAdminOverlayEnabled;
		ARAL_Runtime.GetInstance().SetAdminOverlayEnabled(m_bAdminOverlayEnabled);
	}

	[ButtonAttribute("Start/Load Campaign")]
	void EditorStartOrLoadCampaign()
	{
		if (!Replication.IsServer())
			return;

		bool started = ARAL_Runtime.GetInstance().LoadOrStartCampaign(BuildPreset(), m_bAutoLoadSaveBeforeStart);
		if (started && m_bStartPaused)
			ARAL_Runtime.GetInstance().PauseCampaign();
	}

	[ButtonAttribute("Save Campaign")]
	void EditorSaveCampaign()
	{
		if (!Replication.IsServer())
			return;

		ARAL_Runtime.GetInstance().SaveCampaign();
	}

	[ButtonAttribute("Pause Campaign")]
	void EditorPauseCampaign()
	{
		if (!Replication.IsServer())
			return;

		ARAL_Runtime.GetInstance().PauseCampaign();
	}

	[ButtonAttribute("Resume Campaign")]
	void EditorResumeCampaign()
	{
		if (!Replication.IsServer())
			return;

		ARAL_Runtime.GetInstance().ResumeCampaign();
	}

	[ButtonAttribute("Rebuild Objectives")]
	void EditorRebuildObjectives()
	{
		if (!Replication.IsServer())
			return;

		ARAL_Runtime.GetInstance().RebuildObjectives();
	}

	[ButtonAttribute("Export Setup Report")]
	void EditorExportSetupReport()
	{
		if (!Replication.IsServer())
			return;

		ARAL_Runtime.GetInstance().ExportSetupSnapshotReport(BuildPreset());
	}
}
