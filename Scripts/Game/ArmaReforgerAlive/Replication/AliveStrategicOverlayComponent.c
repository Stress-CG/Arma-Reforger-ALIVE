[ComponentEditorProps(category: "Arma Reforger Alive/Replication", description: "Replicated lightweight strategic overlay state for JIP-safe UI and admin visibility.")]
class AliveStrategicOverlayComponentClass : ScriptComponentClass
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};
		requires.Insert(RplComponent);
		return requires;
	}
}

class AliveStrategicOverlayComponent : ScriptComponent
{
	[RplProp(onRplName: "OnOverlayChanged")]
	protected int m_iGeneration;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected int m_iConnectedPlayerCount;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected int m_iPhysicalProjectionCount;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected int m_iPendingProjectionCount;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected int m_iRawProjectionCount;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected bool m_bAdminOverlayEnabled;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sObjectiveDigest;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sTaskDigest;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sHotProfileDigest;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sHandoffDigest;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sProfileOverlayDigest;

	[RplProp(onRplName: "OnOverlayChanged")]
	protected string m_sReclaimOverlayDigest;

	void ApplyOverlay(AliveStrategicOverlaySnapshot snapshot)
	{
		if (!snapshot)
			return;

		m_iGeneration = snapshot.m_iGeneration;
		m_iConnectedPlayerCount = snapshot.m_iConnectedPlayerCount;
		m_iPhysicalProjectionCount = snapshot.m_iPhysicalProjectionCount;
		m_iPendingProjectionCount = snapshot.m_iPendingProjectionCount;
		m_iRawProjectionCount = snapshot.m_iRawProjectionCount;
		m_bAdminOverlayEnabled = snapshot.m_bAdminOverlayEnabled;
		m_sObjectiveDigest = snapshot.m_sObjectiveDigest;
		m_sTaskDigest = snapshot.m_sTaskDigest;
		m_sHotProfileDigest = snapshot.m_sHotProfileDigest;
		m_sHandoffDigest = snapshot.m_sHandoffDigest;
		m_sProfileOverlayDigest = snapshot.m_sProfileOverlayDigest;
		m_sReclaimOverlayDigest = snapshot.m_sReclaimOverlayDigest;
		OnOverlayChanged();
		Replication.BumpMe();
	}

	void OnOverlayChanged()
	{
	}
}
