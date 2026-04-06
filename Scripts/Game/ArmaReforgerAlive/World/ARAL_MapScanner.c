class ARAL_MapScannerBase : Managed
{
	void BuildObjectives(IEntity owner, ARAL_MissionPreset preset, out array<ref ARAL_Objective> outObjectives)
	{
	}
}

class ARAL_DefaultMapScanner : ARAL_MapScannerBase
{
	protected ref array<ref ARAL_Objective> m_aCollectedObjectives = new array<ref ARAL_Objective>();
	protected ARAL_MissionPreset m_Preset;
	protected int m_iOrdinal;
	protected static const float OBJECTIVE_DEDUPE_DISTANCE = 200.0;

	override void BuildObjectives(IEntity owner, ARAL_MissionPreset preset, out array<ref ARAL_Objective> outObjectives)
	{
		m_aCollectedObjectives.Clear();
		m_Preset = preset;
		m_iOrdinal = 0;

		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), preset.m_fWorldScanRadius, QueryEntitiesCallbackMethod, null, EQueryEntitiesFlags.ALL);

		if (m_aCollectedObjectives.IsEmpty())
		{
			ARAL_Objective fallback = new ARAL_Objective();
			fallback.m_sName = "Bootstrap AO";
			fallback.m_vPosition = owner.GetOrigin();
			fallback.m_fRadius = 350;
			fallback.m_iPriority = 50;
			fallback.m_eKind = ARAL_EObjectiveKind.CUSTOM;
			fallback.m_sId = ARAL_IdUtility.MakeStableId("fallback", fallback.m_sName, fallback.m_vPosition, 0);
			TryInsertObjective(fallback);
		}

		int limit = Math.Min(preset.m_iObjectiveLimit, m_aCollectedObjectives.Count());
		for (int i = 0; i < limit; i++)
			outObjectives.Insert(m_aCollectedObjectives[i].Clone());
	}

	protected bool QueryEntitiesCallbackMethod(IEntity entity)
	{
		if (!entity)
			return true;

		if (m_aCollectedObjectives.Count() >= m_Preset.m_iObjectiveLimit)
			return false;

		ARAL_ObjectiveSeedComponent seedComponent = ARAL_ObjectiveSeedComponent.Cast(entity.FindComponent(ARAL_ObjectiveSeedComponent));
		if (seedComponent && m_Preset.m_bUseAuthorOverrides)
		{
			TryInsertObjective(seedComponent.BuildObjective(entity, m_iOrdinal));
			m_iOrdinal++;
			return true;
		}

		if (!m_Preset.m_bAutoScanMap)
			return true;

		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (!descriptor)
			return true;

		MapItem item = descriptor.Item();
		if (!item)
			return true;

		string displayName = item.GetDisplayName();
		if (displayName == "")
			return true;

		ARAL_Objective objective = new ARAL_Objective();
		objective.m_sName = displayName;
		objective.m_vPosition = item.GetPos();
		objective.m_fRadius = 175;
		objective.m_iPriority = 50;
		objective.m_eKind = ARAL_EObjectiveKind.SETTLEMENT;
		objective.m_sId = ARAL_IdUtility.MakeStableId("auto", displayName, objective.m_vPosition, m_iOrdinal);
		TryInsertObjective(objective);
		m_iOrdinal++;
		return true;
	}

	protected bool TryInsertObjective(ARAL_Objective candidate)
	{
		if (!candidate)
			return false;

		foreach (ARAL_Objective existing : m_aCollectedObjectives)
		{
			if (vector.Distance(existing.m_vPosition, candidate.m_vPosition) > OBJECTIVE_DEDUPE_DISTANCE)
				continue;

			if (candidate.m_bAuthorOverride && !existing.m_bAuthorOverride)
			{
				existing.m_sId = candidate.m_sId;
				existing.m_sName = candidate.m_sName;
				existing.m_vPosition = candidate.m_vPosition;
				existing.m_fRadius = candidate.m_fRadius;
				existing.m_iPriority = candidate.m_iPriority;
				existing.m_eKind = candidate.m_eKind;
				existing.m_sOwnerFactionKey = candidate.m_sOwnerFactionKey;
				existing.m_bAuthorOverride = true;
				return true;
			}

			return false;
		}

		m_aCollectedObjectives.Insert(candidate);
		return true;
	}
}

class ARAL_WorldService : Managed
{
	protected ref ARAL_MapScannerBase m_MapScanner;

	void ARAL_WorldService(ARAL_MapScannerBase mapScanner = null)
	{
		if (mapScanner)
			m_MapScanner = mapScanner;
		else
			m_MapScanner = new ARAL_DefaultMapScanner();
	}

	void BuildWarSeed(IEntity owner, ARAL_MissionPreset preset, ARAL_WarState warState)
	{
		if (!warState)
			return;

		warState.EnsureCollections();
		warState.m_aObjectives.Clear();

		array<ref ARAL_Objective> objectives = {};
		m_MapScanner.BuildObjectives(owner, preset, objectives);

		foreach (ARAL_Objective objective : objectives)
			warState.m_aObjectives.Insert(objective.Clone());
	}
}
