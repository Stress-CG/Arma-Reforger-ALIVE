[ComponentEditorProps(category: "Arma Reforger Alive/Handoff", description: "Lightweight runtime projection component that binds a spawned entity hierarchy to a virtual profile.")]
class AlivePhysicalAgentComponentClass : ScriptComponentClass
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};
		requires.Insert(RplComponent);
		return requires;
	}
}

class AlivePhysicalAgentComponent : ScriptComponent
{
	[RplProp(onRplName: "OnProfileBindingChanged")]
	protected string m_sProfileId;

	[RplProp(onRplName: "OnProfileBindingChanged")]
	protected string m_sTemplateId;

	[RplProp(onRplName: "OnProfileBindingChanged")]
	protected int m_iSpawnGeneration;

	protected RplComponent m_RplComponent;
	protected float m_fCreatedAt;
	protected float m_fLastObservedAt;
	protected float m_fLastEngagedAt;
	protected float m_fEstimatedSpeedMps;
	protected float m_fOrderProgress;
	protected vector m_vOrderAnchorPosition = "0 0 0";
	protected string m_sOrderId;
	protected string m_sObjectiveId;
	protected int m_iDesiredCount;
	protected int m_iAliveCount;
	protected AliveETacticalMode m_eTacticalMode = AliveETacticalMode.None;
	protected bool m_bVehicleOperational = true;
	protected bool m_bVehicleMoving;
	protected bool m_bPlayerOccupancyBlocked;
	protected bool m_bHasEnemyTarget;
	protected bool m_bFiredRecently;
	protected bool m_bTookDamageRecently;
	protected float m_fVehicleHealthNormalized = 1.0;
	protected float m_fVehicleFuelNormalized = 1.0;
	protected bool m_bPendingReclaim;

	override void OnPostInit(IEntity owner)
	{
		m_RplComponent = SCR_EntityHelper.GetEntityRplComponent(owner);
		m_fCreatedAt = System.GetUnixTime();
	}

	void BindToProfile(string profileId, string templateId, int spawnGeneration, int desiredCount)
	{
		m_sProfileId = profileId;
		m_sTemplateId = templateId;
		m_iSpawnGeneration = spawnGeneration;
		m_iDesiredCount = desiredCount;
		m_iAliveCount = desiredCount;
		m_bPendingReclaim = false;
		m_fCreatedAt = System.GetUnixTime();

		if (m_RplComponent)
			Replication.BumpMe();
	}

	void SetDesiredCount(int desiredCount)
	{
		m_iDesiredCount = desiredCount;
		if (m_iAliveCount <= 0)
			m_iAliveCount = desiredCount;
	}

	void ReportAliveCount(int aliveCount)
	{
		m_iAliveCount = Math.Max(aliveCount, 0);
	}

	void ReportVehicleState(bool operational, float healthNormalized, float fuelNormalized)
	{
		m_bVehicleOperational = operational;
		m_fVehicleHealthNormalized = healthNormalized;
		m_fVehicleFuelNormalized = fuelNormalized;
	}

	void SetOrderContext(string orderId, string objectiveId)
	{
		m_sOrderId = orderId;
		m_sObjectiveId = objectiveId;
	}

	void SetOrderProgress(float progressNormalized, vector anchorPosition)
	{
		m_fOrderProgress = progressNormalized;
		m_vOrderAnchorPosition = anchorPosition;
	}

	void SetTacticalState(AliveETacticalMode tacticalMode, float estimatedSpeedMps, bool vehicleMoving, bool playerOccupancyBlocked, bool hasEnemyTarget, bool firedRecently, bool tookDamageRecently)
	{
		m_eTacticalMode = tacticalMode;
		m_fEstimatedSpeedMps = estimatedSpeedMps;
		m_bVehicleMoving = vehicleMoving;
		m_bPlayerOccupancyBlocked = playerOccupancyBlocked;
		m_bHasEnemyTarget = hasEnemyTarget;
		m_bFiredRecently = firedRecently;
		m_bTookDamageRecently = tookDamageRecently;
	}

	float GetOrderProgress()
	{
		return m_fOrderProgress;
	}

	AliveETacticalMode GetTacticalMode()
	{
		return m_eTacticalMode;
	}

	float GetEstimatedSpeedMps()
	{
		return m_fEstimatedSpeedMps;
	}

	bool IsVehicleMoving()
	{
		return m_bVehicleMoving;
	}

	bool IsPlayerOccupancyBlocked()
	{
		return m_bPlayerOccupancyBlocked;
	}

	vector GetOrderAnchorPosition()
	{
		return m_vOrderAnchorPosition;
	}

	void MarkObserved(float now)
	{
		m_fLastObservedAt = now;
	}

	void MarkEngaged(float now)
	{
		m_fLastEngagedAt = now;
	}

	bool CanBeReclaimed(float now, float engagementGraceSeconds, float visibilityGraceSeconds)
	{
		if (m_bPendingReclaim)
			return false;

		if (m_bPlayerOccupancyBlocked)
			return false;

		if (m_bVehicleMoving)
			return false;

		if (m_fLastObservedAt > 0 && now < m_fLastObservedAt + visibilityGraceSeconds)
			return false;

		if (m_fLastEngagedAt > 0 && now < m_fLastEngagedAt + engagementGraceSeconds)
			return false;

		return true;
	}

	void PrepareForReclaim()
	{
		m_bPendingReclaim = true;
	}

	AlivePhysicalSnapshot CaptureSnapshot(IEntity owner)
	{
		AlivePhysicalSnapshot snapshot = new AlivePhysicalSnapshot();
		snapshot.m_sProfileId = m_sProfileId;
		if (owner)
			snapshot.m_vPosition = owner.GetOrigin();

		snapshot.m_iAliveCount = m_iAliveCount;
		snapshot.m_fCapturedAt = System.GetUnixTime();
		snapshot.m_OrderProgress.m_sOrderId = m_sOrderId;
		snapshot.m_OrderProgress.m_fCompletionNormalized = m_fOrderProgress;
		snapshot.m_OrderProgress.m_vAnchorPosition = m_vOrderAnchorPosition;
		snapshot.m_VehicleState.m_bHasVehicle = false;
		snapshot.m_VehicleState.m_bOperational = m_bVehicleOperational;
		snapshot.m_VehicleState.m_fHealthNormalized = m_fVehicleHealthNormalized;
		snapshot.m_VehicleState.m_fFuelNormalized = m_fVehicleFuelNormalized;
		return snapshot;
	}

	string GetProfileId()
	{
		return m_sProfileId;
	}

	int GetSpawnGeneration()
	{
		return m_iSpawnGeneration;
	}

	RplComponent GetRplComponent()
	{
		return m_RplComponent;
	}

	bool IsPendingReclaim()
	{
		return m_bPendingReclaim;
	}

	void OnProfileBindingChanged()
	{
	}
}
