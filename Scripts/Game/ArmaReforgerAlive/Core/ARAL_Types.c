enum ARAL_EWarPhase
{
	UNCONFIGURED,
	SETUP,
	RUNNING,
	PAUSED,
	ENDED
}

enum ARAL_EObjectiveKind
{
	NONE,
	SETTLEMENT,
	MILITARY_BASE,
	LOGISTICS,
	AIRFIELD,
	PORT,
	CUSTOM
}

enum ARAL_EProfileState
{
	VIRTUAL,
	QUEUED_SPAWN,
	MATERIALIZED,
	QUEUED_DESPAWN
}

enum ARAL_EOrderType
{
	HOLD,
	ATTACK,
	DEFEND,
	REDEPLOY,
	REINFORCE
}

enum ARAL_ETaskStatus
{
	PENDING,
	ACTIVE,
	COMPLETED,
	FAILED
}

enum ARAL_EMaterializationState
{
	NONE,
	PENDING,
	READY,
	VIRTUALIZED
}

class ARAL_ActivationBudget : Managed
{
	float m_fInfantrySpawnRadius = 1200;
	float m_fVehicleSpawnRadius = 1800;
	float m_fDespawnRadius = 2200;
	float m_fSmoothSpawnSeconds = 0.30;
	int m_iActiveLimiter = 72;

	ref ARAL_ActivationBudget Clone()
	{
		ARAL_ActivationBudget copy = new ARAL_ActivationBudget();
		copy.m_fInfantrySpawnRadius = m_fInfantrySpawnRadius;
		copy.m_fVehicleSpawnRadius = m_fVehicleSpawnRadius;
		copy.m_fDespawnRadius = m_fDespawnRadius;
		copy.m_fSmoothSpawnSeconds = m_fSmoothSpawnSeconds;
		copy.m_iActiveLimiter = m_iActiveLimiter;
		return copy;
	}
}

class ARAL_FactionChoice : Managed
{
	string m_sKey;
	string m_sName;
	int m_iIndex = -1;
	bool m_bPlayable = true;

	ref ARAL_FactionChoice Clone()
	{
		ARAL_FactionChoice copy = new ARAL_FactionChoice();
		copy.m_sKey = m_sKey;
		copy.m_sName = m_sName;
		copy.m_iIndex = m_iIndex;
		copy.m_bPlayable = m_bPlayable;
		return copy;
	}
}

class ARAL_CatalogEntry : Managed
{
	string m_sName;
	string m_sPath;

	ref ARAL_CatalogEntry Clone()
	{
		ARAL_CatalogEntry copy = new ARAL_CatalogEntry();
		copy.m_sName = m_sName;
		copy.m_sPath = m_sPath;
		return copy;
	}
}

class ARAL_Objective : Managed
{
	string m_sId;
	string m_sName;
	vector m_vPosition = "0 0 0";
	float m_fRadius = 250;
	int m_iPriority = 50;
	ARAL_EObjectiveKind m_eKind = ARAL_EObjectiveKind.CUSTOM;
	string m_sOwnerFactionKey;
	bool m_bAuthorOverride;
	bool m_bEnabled = true;

	ref ARAL_Objective Clone()
	{
		ARAL_Objective copy = new ARAL_Objective();
		copy.m_sId = m_sId;
		copy.m_sName = m_sName;
		copy.m_vPosition = m_vPosition;
		copy.m_fRadius = m_fRadius;
		copy.m_iPriority = m_iPriority;
		copy.m_eKind = m_eKind;
		copy.m_sOwnerFactionKey = m_sOwnerFactionKey;
		copy.m_bAuthorOverride = m_bAuthorOverride;
		copy.m_bEnabled = m_bEnabled;
		return copy;
	}
}

class ARAL_Order : Managed
{
	string m_sId;
	string m_sProfileId;
	string m_sObjectiveId;
	ARAL_EOrderType m_eType = ARAL_EOrderType.HOLD;
	int m_iPriority = 50;
	float m_fIssuedAt;
	float m_fDueAt;
	float m_fProgressNormalized;
	vector m_vLastKnownPosition = "0 0 0";
	float m_fLastPhysicalUpdateAt;

	ref ARAL_Order Clone()
	{
		ARAL_Order copy = new ARAL_Order();
		copy.m_sId = m_sId;
		copy.m_sProfileId = m_sProfileId;
		copy.m_sObjectiveId = m_sObjectiveId;
		copy.m_eType = m_eType;
		copy.m_iPriority = m_iPriority;
		copy.m_fIssuedAt = m_fIssuedAt;
		copy.m_fDueAt = m_fDueAt;
		copy.m_fProgressNormalized = m_fProgressNormalized;
		copy.m_vLastKnownPosition = m_vLastKnownPosition;
		copy.m_fLastPhysicalUpdateAt = m_fLastPhysicalUpdateAt;
		return copy;
	}
}

class ARAL_ForceProfile : Managed
{
	string m_sId;
	string m_sFactionKey;
	string m_sTemplateId;
	string m_sDisplayName;
	string m_sAnchorObjectiveId;
	string m_sActiveOrderId;
	vector m_vVirtualPosition = "0 0 0";
	int m_iStrength = 8;
	int m_iSpawnPriority = 50;
	int m_iMaterializedCount;
	float m_fLastPhysicalUpdateAt;
	bool m_bVehicleProfile;
	bool m_bVehicleOperational = true;
	float m_fVehicleHealthNormalized = 1.0;
	float m_fVehicleFuelNormalized = 1.0;
	ARAL_EProfileState m_eState = ARAL_EProfileState.VIRTUAL;

	ref ARAL_ForceProfile Clone()
	{
		ARAL_ForceProfile copy = new ARAL_ForceProfile();
		copy.m_sId = m_sId;
		copy.m_sFactionKey = m_sFactionKey;
		copy.m_sTemplateId = m_sTemplateId;
		copy.m_sDisplayName = m_sDisplayName;
		copy.m_sAnchorObjectiveId = m_sAnchorObjectiveId;
		copy.m_sActiveOrderId = m_sActiveOrderId;
		copy.m_vVirtualPosition = m_vVirtualPosition;
		copy.m_iStrength = m_iStrength;
		copy.m_iSpawnPriority = m_iSpawnPriority;
		copy.m_iMaterializedCount = m_iMaterializedCount;
		copy.m_fLastPhysicalUpdateAt = m_fLastPhysicalUpdateAt;
		copy.m_bVehicleProfile = m_bVehicleProfile;
		copy.m_bVehicleOperational = m_bVehicleOperational;
		copy.m_fVehicleHealthNormalized = m_fVehicleHealthNormalized;
		copy.m_fVehicleFuelNormalized = m_fVehicleFuelNormalized;
		copy.m_eState = m_eState;
		return copy;
	}
}

class ARAL_MaterializationRequest : Managed
{
	string m_sProfileId;
	string m_sFactionKey;
	string m_sTemplateId;
	string m_sDisplayName;
	vector m_vSpawnPosition = "0 0 0";
	int m_iDesiredCount;
	bool m_bVehicleProfile;
	ARAL_EMaterializationState m_eState = ARAL_EMaterializationState.NONE;

	ref ARAL_MaterializationRequest Clone()
	{
		ARAL_MaterializationRequest copy = new ARAL_MaterializationRequest();
		copy.m_sProfileId = m_sProfileId;
		copy.m_sFactionKey = m_sFactionKey;
		copy.m_sTemplateId = m_sTemplateId;
		copy.m_sDisplayName = m_sDisplayName;
		copy.m_vSpawnPosition = m_vSpawnPosition;
		copy.m_iDesiredCount = m_iDesiredCount;
		copy.m_bVehicleProfile = m_bVehicleProfile;
		copy.m_eState = m_eState;
		return copy;
	}
}

class ARAL_TaskRecord : Managed
{
	string m_sId;
	string m_sTitle;
	string m_sDetails;
	string m_sObjectiveId;
	string m_sFactionKey;
	ARAL_ETaskStatus m_eStatus = ARAL_ETaskStatus.PENDING;

	ref ARAL_TaskRecord Clone()
	{
		ARAL_TaskRecord copy = new ARAL_TaskRecord();
		copy.m_sId = m_sId;
		copy.m_sTitle = m_sTitle;
		copy.m_sDetails = m_sDetails;
		copy.m_sObjectiveId = m_sObjectiveId;
		copy.m_sFactionKey = m_sFactionKey;
		copy.m_eStatus = m_eStatus;
		return copy;
	}
}

class ARAL_MissionPreset : Managed
{
	string m_sName = "Default Campaign";
	string m_sDescription = "Reusable Reforger ALIVE configuration.";
	string m_sPlayerFactionKey;
	string m_sEnemyFactionKey;
	ref ARAL_ActivationBudget m_ActivationBudget = new ARAL_ActivationBudget();
	bool m_bAutoScanMap = true;
	bool m_bUseAuthorOverrides = true;
	int m_iObjectiveLimit = 48;
	int m_iInitialProfilesPerSide = 12;
	float m_fWorldScanRadius = 20000;
	float m_fSaveIntervalSeconds = 120;

	void EnsureDefaults()
	{
		if (!m_ActivationBudget)
			m_ActivationBudget = new ARAL_ActivationBudget();
	}

	ref ARAL_MissionPreset Clone()
	{
		ARAL_MissionPreset copy = new ARAL_MissionPreset();
		copy.m_sName = m_sName;
		copy.m_sDescription = m_sDescription;
		copy.m_sPlayerFactionKey = m_sPlayerFactionKey;
		copy.m_sEnemyFactionKey = m_sEnemyFactionKey;
		copy.m_ActivationBudget = m_ActivationBudget.Clone();
		copy.m_bAutoScanMap = m_bAutoScanMap;
		copy.m_bUseAuthorOverrides = m_bUseAuthorOverrides;
		copy.m_iObjectiveLimit = m_iObjectiveLimit;
		copy.m_iInitialProfilesPerSide = m_iInitialProfilesPerSide;
		copy.m_fWorldScanRadius = m_fWorldScanRadius;
		copy.m_fSaveIntervalSeconds = m_fSaveIntervalSeconds;
		return copy;
	}
}

class ARAL_SetupSnapshot : Managed
{
	ref ARAL_MissionPreset m_DraftPreset = new ARAL_MissionPreset();
	ref array<ref ARAL_FactionChoice> m_aAvailableFactions = new array<ref ARAL_FactionChoice>();
	ref array<ref ARAL_CatalogEntry> m_aPresetCatalog = new array<ref ARAL_CatalogEntry>();
	ref array<ref ARAL_CatalogEntry> m_aSaveCatalog = new array<ref ARAL_CatalogEntry>();
	string m_sResolvedPlayerFactionKey;
	string m_sResolvedEnemyFactionKey;

	ref ARAL_SetupSnapshot Clone()
	{
		ARAL_SetupSnapshot copy = new ARAL_SetupSnapshot();
		copy.m_DraftPreset = m_DraftPreset.Clone();
		copy.m_sResolvedPlayerFactionKey = m_sResolvedPlayerFactionKey;
		copy.m_sResolvedEnemyFactionKey = m_sResolvedEnemyFactionKey;

		foreach (ARAL_FactionChoice choice : m_aAvailableFactions)
			copy.m_aAvailableFactions.Insert(choice.Clone());

		foreach (ARAL_CatalogEntry entry : m_aPresetCatalog)
			copy.m_aPresetCatalog.Insert(entry.Clone());

		foreach (ARAL_CatalogEntry entrySave : m_aSaveCatalog)
			copy.m_aSaveCatalog.Insert(entrySave.Clone());

		return copy;
	}
}

class ARAL_WarState : Managed
{
	int m_iSchemaVersion = 1;
	string m_sPresetName;
	ARAL_EWarPhase m_ePhase = ARAL_EWarPhase.SETUP;
	ref array<ref ARAL_Objective> m_aObjectives = new array<ref ARAL_Objective>();
	ref array<ref ARAL_ForceProfile> m_aProfiles = new array<ref ARAL_ForceProfile>();
	ref array<ref ARAL_MaterializationRequest> m_aMaterializationRequests = new array<ref ARAL_MaterializationRequest>();
	ref array<ref ARAL_Order> m_aOrders = new array<ref ARAL_Order>();
	ref array<ref ARAL_TaskRecord> m_aTasks = new array<ref ARAL_TaskRecord>();

	void EnsureCollections()
	{
		if (!m_aObjectives)
			m_aObjectives = new array<ref ARAL_Objective>();
		if (!m_aProfiles)
			m_aProfiles = new array<ref ARAL_ForceProfile>();
		if (!m_aMaterializationRequests)
			m_aMaterializationRequests = new array<ref ARAL_MaterializationRequest>();
		if (!m_aOrders)
			m_aOrders = new array<ref ARAL_Order>();
		if (!m_aTasks)
			m_aTasks = new array<ref ARAL_TaskRecord>();
	}

	ref ARAL_WarState Clone()
	{
		ARAL_WarState copy = new ARAL_WarState();
		copy.m_iSchemaVersion = m_iSchemaVersion;
		copy.m_sPresetName = m_sPresetName;
		copy.m_ePhase = m_ePhase;

		foreach (ARAL_Objective objective : m_aObjectives)
			copy.m_aObjectives.Insert(objective.Clone());

		foreach (ARAL_ForceProfile profile : m_aProfiles)
			copy.m_aProfiles.Insert(profile.Clone());

		foreach (ARAL_MaterializationRequest request : m_aMaterializationRequests)
			copy.m_aMaterializationRequests.Insert(request.Clone());

		foreach (ARAL_Order order : m_aOrders)
			copy.m_aOrders.Insert(order.Clone());

		foreach (ARAL_TaskRecord task : m_aTasks)
			copy.m_aTasks.Insert(task.Clone());

		return copy;
	}
}

class ARAL_PersistenceEnvelope : Managed
{
	int m_iSchemaVersion = 1;
	float m_fSavedAt;
	ref ARAL_MissionPreset m_Preset = new ARAL_MissionPreset();
	ref ARAL_WarState m_WarState = new ARAL_WarState();

	void EnsureDefaults()
	{
		if (!m_Preset)
			m_Preset = new ARAL_MissionPreset();

		if (!m_WarState)
			m_WarState = new ARAL_WarState();

		m_Preset.EnsureDefaults();
		m_WarState.EnsureCollections();
	}
}

class ARAL_ProfileActivationChange : Managed
{
	string m_sProfileId;
	ARAL_EProfileState m_eTargetState;
}
