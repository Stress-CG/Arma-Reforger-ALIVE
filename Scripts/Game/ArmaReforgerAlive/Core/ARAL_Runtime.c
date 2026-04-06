class ARAL_Runtime : Managed
{
	protected static ref ARAL_Runtime s_Instance;

	protected IEntity m_BootstrapOwner;
	protected ARAL_RuntimeStateComponent m_RuntimeState;
	protected AliveStrategicOverlayComponent m_StrategicOverlay;
	protected ref ARAL_PersistenceService m_PersistenceService;
	protected ref ARAL_StatusReportService m_StatusReportService;
	protected ref ARAL_SetupService m_SetupService;
	protected ref ARAL_SetupReportService m_SetupReportService;
	protected ref ARAL_WorldService m_WorldService;
	protected ref ARAL_SimulationService m_SimulationService;
	protected ref AliveStrategicOverlayService m_StrategicOverlayService;
	protected ref ARAL_MissionPreset m_ActivePreset;
	protected ref ARAL_WarState m_WarState;
	protected ref array<ref ARAL_FactionChoice> m_aAvailableFactions = new array<ref ARAL_FactionChoice>();

	protected bool m_bStarted;
	protected bool m_bWriteStatusReports;
	protected bool m_bAdminOverlayEnabled = ARAL_ProjectPolicy.ADMIN_OVERLAY_ONLY;
	protected float m_fSaveAccumulator;
	protected float m_fSummaryAccumulator;
	protected float m_fOverlayAccumulator;
	protected static const float SUMMARY_UPDATE_PERIOD_SECONDS = 0.50;
	protected static const float OVERLAY_UPDATE_PERIOD_SECONDS = 2.0;

	static ARAL_Runtime GetInstance()
	{
		if (!s_Instance)
			s_Instance = new ARAL_Runtime();

		return s_Instance;
	}

	void ARAL_Runtime()
	{
		m_PersistenceService = new ARAL_PersistenceService();
		m_StatusReportService = new ARAL_StatusReportService();
		m_SetupService = new ARAL_SetupService(m_PersistenceService);
		m_SetupReportService = new ARAL_SetupReportService();
		m_WorldService = new ARAL_WorldService();
		m_SimulationService = new ARAL_SimulationService();
		m_StrategicOverlayService = new AliveStrategicOverlayService();
	}

	void Initialize(IEntity owner, ARAL_RuntimeStateComponent runtimeState)
	{
		m_BootstrapOwner = owner;
		m_RuntimeState = runtimeState;
		m_StrategicOverlay = AliveStrategicOverlayComponent.Cast(owner.FindComponent(AliveStrategicOverlayComponent));
		RefreshAvailableFactions();
	}

	void SetWriteStatusReports(bool writeStatusReports)
	{
		m_bWriteStatusReports = writeStatusReports;
	}

	void SetAdminOverlayEnabled(bool adminOverlayEnabled)
	{
		m_bAdminOverlayEnabled = adminOverlayEnabled;
	}

	bool StartCampaign(ARAL_MissionPreset preset)
	{
		if (!preset || !m_BootstrapOwner)
			return false;

		if (m_bStarted)
			m_SimulationService.ResetRuntime(true);

		RefreshAvailableFactions();
		m_SetupService.NormalizePreset(preset, m_aAvailableFactions);
		m_ActivePreset = preset.Clone();
		m_WarState = new ARAL_WarState();
		m_WarState.m_sPresetName = m_ActivePreset.m_sName;
		m_WarState.m_ePhase = ARAL_EWarPhase.RUNNING;
		m_WarState.EnsureCollections();

		m_WorldService.BuildWarSeed(m_BootstrapOwner, m_ActivePreset, m_WarState);
		m_SimulationService.InitializeWar(m_ActivePreset, m_WarState);
		m_PersistenceService.SavePreset(m_ActivePreset);
		m_PersistenceService.SaveWarState(m_ActivePreset, m_WarState);

		m_bStarted = true;
		m_fSaveAccumulator = 0;
		m_fSummaryAccumulator = 0;
		m_fOverlayAccumulator = 0;
		WriteStatusReport();
		RefreshSummary(true);
		RefreshOverlay(true);
		ARAL_Log.Info(string.Format("Campaign '%1' started with %2 objectives.", m_ActivePreset.m_sName, m_WarState.m_aObjectives.Count()));
		return true;
	}

	bool LoadOrStartCampaign(ARAL_MissionPreset defaultPreset, bool preferSave = true)
	{
		if (preferSave && defaultPreset && m_SetupService.HasSaveForPreset(defaultPreset.m_sName))
		{
			if (LoadCampaign(defaultPreset.m_sName))
				return true;
		}

		return StartCampaign(defaultPreset);
	}

	bool LoadCampaign(string presetName)
	{
		if (m_bStarted)
			m_SimulationService.ResetRuntime(true);

		ARAL_MissionPreset preset;
		ARAL_WarState warState;
		if (!m_PersistenceService.LoadWarState(presetName, preset, warState))
			return false;

		RefreshAvailableFactions();
		m_SetupService.NormalizePreset(preset, m_aAvailableFactions);
		m_ActivePreset = preset;
		m_WarState = warState;
		m_SimulationService.ResetRuntime(false);
		m_bStarted = true;
		m_fSaveAccumulator = 0;
		m_fSummaryAccumulator = 0;
		m_fOverlayAccumulator = 0;
		WriteStatusReport();
		RefreshSummary(true);
		RefreshOverlay(true);
		ARAL_Log.Info(string.Format("Campaign '%1' loaded from persistence.", presetName));
		return true;
	}

	bool SaveCampaign()
	{
		if (!m_bStarted || !m_ActivePreset || !m_WarState)
			return false;

		ARAL_WarState persistenceState = m_SimulationService.BuildPersistenceSnapshot(m_WarState);
		if (!persistenceState)
			persistenceState = m_WarState;

		bool result = m_PersistenceService.SaveWarState(m_ActivePreset, persistenceState);
		if (result)
			WriteStatusReport();

		return result;
	}

	bool PauseCampaign()
	{
		if (!m_WarState)
			return false;

		m_WarState.m_ePhase = ARAL_EWarPhase.PAUSED;
		RefreshSummary(true);
		return true;
	}

	bool ResumeCampaign()
	{
		if (!m_WarState)
			return false;

		m_WarState.m_ePhase = ARAL_EWarPhase.RUNNING;
		RefreshSummary(true);
		return true;
	}

	bool EndCampaign()
	{
		if (!m_WarState)
			return false;

		m_WarState.m_ePhase = ARAL_EWarPhase.ENDED;
		m_SimulationService.ResetRuntime(true);
		SaveCampaign();
		RefreshSummary(true);
		RefreshOverlay(true);
		return true;
	}

	bool RebuildObjectives()
	{
		if (!m_ActivePreset || !m_WarState || !m_BootstrapOwner)
			return false;

		m_SimulationService.ResetRuntime(true);
		m_WorldService.BuildWarSeed(m_BootstrapOwner, m_ActivePreset, m_WarState);
		m_SimulationService.InitializeWar(m_ActivePreset, m_WarState);
		RefreshSummary(true);
		RefreshOverlay(true);
		return true;
	}

	void Tick(float timeSlice)
	{
		if (!m_bStarted || !m_ActivePreset || !m_WarState)
			return;

		array<vector> spawnSources = {};
		CollectSpawnSources(spawnSources);
		m_SimulationService.Tick(m_BootstrapOwner, m_WarState, m_ActivePreset, timeSlice, spawnSources);

		m_fSaveAccumulator += timeSlice;
		m_fSummaryAccumulator += timeSlice;
		m_fOverlayAccumulator += timeSlice;
		if (m_fSaveAccumulator >= m_ActivePreset.m_fSaveIntervalSeconds)
		{
			m_fSaveAccumulator = 0;
			SaveCampaign();
		}

		if (m_fSummaryAccumulator >= SUMMARY_UPDATE_PERIOD_SECONDS)
		{
			m_fSummaryAccumulator = 0;
			RefreshSummary();
		}
		if (m_fOverlayAccumulator >= OVERLAY_UPDATE_PERIOD_SECONDS)
		{
			m_fOverlayAccumulator = 0;
			RefreshOverlay();
		}
	}

	void RefreshSummary(bool force = false)
	{
		if (!m_RuntimeState || !m_ActivePreset || !m_WarState)
			return;

		if (force)
			m_fSummaryAccumulator = 0;

		m_RuntimeState.ApplySummary(m_ActivePreset, m_WarState, GetConnectedPlayerCount(), m_SimulationService.GetPhysicalProjectionCount(), m_SimulationService.GetPendingProjectionCount(), m_SimulationService.GetRawProjectionCount());
	}

	void GetAvailableFactions(out array<ref ARAL_FactionChoice> outChoices)
	{
		outChoices.Clear();

		foreach (ARAL_FactionChoice choice : m_aAvailableFactions)
			outChoices.Insert(choice.Clone());
	}

	ref ARAL_SetupSnapshot BuildSetupSnapshot(ARAL_MissionPreset draftPreset = null)
	{
		RefreshAvailableFactions();
		return m_SetupService.BuildSetupSnapshot(draftPreset);
	}

	bool ExportSetupSnapshotReport(ARAL_MissionPreset draftPreset = null)
	{
		ARAL_SetupSnapshot snapshot = BuildSetupSnapshot(draftPreset);
		return m_SetupReportService.WriteSnapshotReport(snapshot);
	}

	ARAL_MissionPreset GetActivePreset()
	{
		return m_ActivePreset;
	}

	ARAL_WarState GetWarState()
	{
		return m_WarState;
	}

	protected void CollectSpawnSources(out array<vector> outSources)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> players = {};
			playerManager.GetPlayers(players);

			foreach (int playerId : players)
			{
				IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
				if (!controlledEntity)
					continue;

				outSources.Insert(controlledEntity.GetOrigin());
			}
		}

		if (outSources.IsEmpty() && m_BootstrapOwner)
			outSources.Insert(m_BootstrapOwner.GetOrigin());
	}

	protected void RefreshAvailableFactions()
	{
		m_aAvailableFactions.Clear();
		m_SetupService.GetAvailableFactions(m_aAvailableFactions);
	}

	protected void WriteStatusReport()
	{
		if (!m_bWriteStatusReports)
			return;

		m_StatusReportService.WriteReport(
			m_ActivePreset,
			m_WarState,
			GetConnectedPlayerCount(),
			m_SimulationService.GetPhysicalProjectionCount(),
			m_SimulationService.GetPendingProjectionCount(),
			m_SimulationService.GetRawProjectionCount(),
			m_SimulationService.BuildHandoffDebugSummary(m_WarState),
			m_bAdminOverlayEnabled ? m_SimulationService.BuildAdminProfileOverlay(m_WarState) : "",
			m_bAdminOverlayEnabled ? m_SimulationService.BuildAdminReclaimOverlay(m_WarState) : ""
		);
	}

	protected void RefreshOverlay(bool force = false)
	{
		if (!m_StrategicOverlay || !m_ActivePreset || !m_WarState)
			return;

		if (force)
			m_fOverlayAccumulator = 0;

		AliveStrategicOverlaySnapshot snapshot = m_StrategicOverlayService.BuildSnapshot(
			m_WarState,
			GetConnectedPlayerCount(),
			m_SimulationService.GetPhysicalProjectionCount(),
			m_SimulationService.GetPendingProjectionCount(),
			m_SimulationService.GetRawProjectionCount(),
			m_bAdminOverlayEnabled,
			m_SimulationService.BuildHandoffDebugSummary(m_WarState),
			m_bAdminOverlayEnabled ? m_SimulationService.BuildAdminProfileOverlay(m_WarState) : "Overlay disabled",
			m_bAdminOverlayEnabled ? m_SimulationService.BuildAdminReclaimOverlay(m_WarState) : "Overlay disabled"
		);
		m_StrategicOverlay.ApplyOverlay(snapshot);
	}

	protected int GetConnectedPlayerCount()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> players = {};
		playerManager.GetPlayers(players);
		return players.Count();
	}
}
