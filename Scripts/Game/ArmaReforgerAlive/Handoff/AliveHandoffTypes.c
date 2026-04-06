enum AliveEHandoffState
{
	Virtual,
	PendingSpawn,
	Spawning,
	Physical,
	PendingReclaim,
	Reclaiming,
	Destroyed
}

enum AliveETacticalMode
{
	None,
	Hold,
	Move,
	Defend,
	Patrol,
	LimitedAssault,
	MountedMovement,
	CombatPatrol,
	Transport
}

class AliveRelevanceContext : Managed
{
	float m_fClosestDistanceSq = 3.402823466e+38;
	bool m_bSpawnRelevant;
	bool m_bStillRelevant;
	bool m_bPlayerVisible;
	bool m_bRecentlyVisible;
	bool m_bRecentlyEngaged;
	vector m_vBestSpawnOrigin = "0 0 0";

	bool CanSpawn()
	{
		return m_bSpawnRelevant;
	}

	bool CanReclaim()
	{
		return !m_bStillRelevant && !m_bPlayerVisible && !m_bRecentlyVisible && !m_bRecentlyEngaged;
	}
}

class AliveSpawnSpec : Managed
{
	string m_sProfileId;
	string m_sFactionKey;
	string m_sTemplateId;
	string m_sDisplayName;
	string m_sOrderId;
	ResourceName m_sRootPrefab = string.Empty;
	ref array<ResourceName> m_aMemberPrefabs = new array<ResourceName>();
	vector m_mTransform[4];
	int m_iDesiredCount;
	int m_iSpawnGeneration;
	bool m_bVehicleProfile;

	void AliveSpawnSpec()
	{
		m_mTransform[0] = "1 0 0";
		m_mTransform[1] = "0 1 0";
		m_mTransform[2] = "0 0 1";
		m_mTransform[3] = "0 0 0";
	}
}

class AliveOrderProgressSnapshot : Managed
{
	string m_sOrderId;
	float m_fCompletionNormalized;
	vector m_vAnchorPosition = "0 0 0";
}

class AliveVehicleSnapshot : Managed
{
	bool m_bHasVehicle;
	bool m_bOperational = true;
	float m_fHealthNormalized = 1.0;
	float m_fFuelNormalized = 1.0;
}

class AlivePhysicalSnapshot : Managed
{
	string m_sProfileId;
	vector m_vPosition = "0 0 0";
	int m_iAliveCount;
	float m_fCapturedAt;
	ref AliveOrderProgressSnapshot m_OrderProgress = new AliveOrderProgressSnapshot();
	ref AliveVehicleSnapshot m_VehicleState = new AliveVehicleSnapshot();
}

class AliveTacticalUpdateSnapshot : Managed
{
	ref AliveOrderProgressSnapshot m_OrderProgress = new AliveOrderProgressSnapshot();
	AliveETacticalMode m_eTacticalMode = AliveETacticalMode.None;
	float m_fEstimatedSpeedMps;
	bool m_bVehicleMoving;
	bool m_bPlayerOccupancyBlocked;
	bool m_bHasEnemyTarget;
	bool m_bFiredRecently;
	bool m_bTookDamageRecently;
	string m_sReclaimBlockReason;
}

class AliveProfileBinding : Managed
{
	string m_sProfileId;
	EntityID m_EntityId;
	RplId m_RplId;
	IEntity m_Entity;
	int m_iSpawnGeneration;
	float m_fCreatedAt;
	bool m_bRawProjectionFallback;
	AliveEHandoffState m_eState = AliveEHandoffState.Virtual;
}

class AliveProfileRuntimeRecord : Managed
{
	string m_sProfileId;
	AliveEHandoffState m_eHandoffState = AliveEHandoffState.Virtual;
	AliveETacticalMode m_eLastTacticalMode = AliveETacticalMode.None;
	float m_fStateChangedAt;
	float m_fReservationUntil;
	float m_fSpawnCooldownUntil;
	float m_fReclaimCooldownUntil;
	float m_fNoReclaimUntil;
	float m_fLastVisibleAt;
	float m_fLastEngagedAt;
	float m_fEstimatedSpeedMps;
	int m_iSpawnGeneration;
	bool m_bReservationHeld;
	bool m_bVehicleMoving;
	bool m_bPlayerOccupancyBlocked;
	bool m_bHasEnemyTarget;
	bool m_bFiredRecently;
	bool m_bTookDamageRecently;
	string m_sLastReclaimBlockReason;
}
