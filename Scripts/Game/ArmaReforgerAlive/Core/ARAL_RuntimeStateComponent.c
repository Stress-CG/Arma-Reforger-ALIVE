[ComponentEditorProps(category: "Arma Reforger Alive/Core", description: "Replicated runtime summary for Reforger ALIVE.")]
class ARAL_RuntimeStateComponentClass : ScriptComponentClass
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};
		requires.Insert(RplComponent);
		return requires;
	}
}

class ARAL_RuntimeStateComponent : ScriptComponent
{
	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iPhase;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected string m_sPresetName;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected string m_sPlayerFactionKey;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected string m_sEnemyFactionKey;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iObjectiveCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iProfileCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iActiveProfiles;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iActiveLimiter;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iOrderCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iTaskCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iMaterializationReadyCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iPhysicalProjectionCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iPendingProjectionCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iRawProjectionCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected int m_iConnectedPlayerCount;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected string m_sPrimaryTaskTitle;

	[RplProp(onRplName: "OnSummaryChanged")]
	protected string m_sStatusText;

	void ApplySummary(ARAL_MissionPreset preset, ARAL_WarState warState, int connectedPlayerCount = 0, int physicalProjectionCount = 0, int pendingProjectionCount = 0, int rawProjectionCount = 0)
	{
		if (!preset || !warState)
			return;

		m_iPhase = warState.m_ePhase;
		m_sPresetName = preset.m_sName;
		m_sPlayerFactionKey = preset.m_sPlayerFactionKey;
		m_sEnemyFactionKey = preset.m_sEnemyFactionKey;
		m_iObjectiveCount = warState.m_aObjectives.Count();
		m_iProfileCount = warState.m_aProfiles.Count();
		m_iActiveProfiles = ARAL_WarStateUtility.CountProfilesInState(warState, ARAL_EProfileState.MATERIALIZED);
		m_iActiveLimiter = preset.m_ActivationBudget.m_iActiveLimiter;
		m_iOrderCount = warState.m_aOrders.Count();
		m_iTaskCount = warState.m_aTasks.Count();
		m_iMaterializationReadyCount = ARAL_WarStateUtility.CountMaterializationRequestsInState(warState, ARAL_EMaterializationState.READY);
		m_iConnectedPlayerCount = connectedPlayerCount;
		m_iPhysicalProjectionCount = physicalProjectionCount;
		m_iPendingProjectionCount = pendingProjectionCount;
		m_iRawProjectionCount = rawProjectionCount;

		m_sPrimaryTaskTitle = "";
		if (!warState.m_aTasks.IsEmpty())
			m_sPrimaryTaskTitle = warState.m_aTasks[0].m_sTitle;

		m_sStatusText = string.Format("%1 | players=%2 | active=%3/%4 | physical=%5 | raw=%6 | pending=%7", ARAL_EnumUtility.WarPhaseToString(warState.m_ePhase), m_iConnectedPlayerCount, m_iActiveProfiles, m_iActiveLimiter, m_iPhysicalProjectionCount, m_iRawProjectionCount, m_iPendingProjectionCount);
		OnSummaryChanged();
		Replication.BumpMe();
	}

	void OnSummaryChanged()
	{
	}
}
