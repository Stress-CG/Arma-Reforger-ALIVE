class ARAL_ProfileTemplate : Managed
{
	string m_sId;
	string m_sFactionKey;
	string m_sDisplayName;
	int m_iStrength = 8;
	int m_iSpawnPriority = 50;
	bool m_bVehicleProfile;

	ref ARAL_ProfileTemplate Clone()
	{
		ARAL_ProfileTemplate copy = new ARAL_ProfileTemplate();
		copy.m_sId = m_sId;
		copy.m_sFactionKey = m_sFactionKey;
		copy.m_sDisplayName = m_sDisplayName;
		copy.m_iStrength = m_iStrength;
		copy.m_iSpawnPriority = m_iSpawnPriority;
		copy.m_bVehicleProfile = m_bVehicleProfile;
		return copy;
	}
}

class ARAL_FactionTemplateService : Managed
{
	protected ref map<string, ref array<ref ARAL_ProfileTemplate>> m_mTemplatesByFaction = new map<string, ref array<ref ARAL_ProfileTemplate>>();

	void ARAL_FactionTemplateService()
	{
		BuildVanillaTemplates();
	}

	ref ARAL_ProfileTemplate GetTemplateForIndex(string factionKey, int ordinal)
	{
		array<ref ARAL_ProfileTemplate> templates = m_mTemplatesByFaction.Get(factionKey);
		if (!templates || templates.IsEmpty())
		{
			EnsureFallbackFactionTemplates(factionKey);
			templates = m_mTemplatesByFaction.Get(factionKey);
		}

		int index = ordinal % templates.Count();
		return templates[index].Clone();
	}

	protected void BuildVanillaTemplates()
	{
		ref array<ref ARAL_ProfileTemplate> usTemplates = {};
		usTemplates.Insert(BuildTemplate("us_rifle", "US Rifle Squad", "US", 8, 70, false));
		usTemplates.Insert(BuildTemplate("us_weapons", "US Weapons Squad", "US", 6, 75, false));
		usTemplates.Insert(BuildTemplate("us_recon", "US Recon Team", "US", 4, 80, false));
		usTemplates.Insert(BuildTemplate("us_motor", "US Motorized Patrol", "US", 6, 85, true));
		usTemplates.Insert(BuildTemplate("us_armor", "US Armor Section", "US", 4, 90, true));
		m_mTemplatesByFaction.Set("US", usTemplates);

		ref array<ref ARAL_ProfileTemplate> ussrTemplates = {};
		ussrTemplates.Insert(BuildTemplate("ussr_rifle", "USSR Rifle Squad", "USSR", 8, 70, false));
		ussrTemplates.Insert(BuildTemplate("ussr_weapons", "USSR Weapons Squad", "USSR", 6, 75, false));
		ussrTemplates.Insert(BuildTemplate("ussr_recon", "USSR Recon Team", "USSR", 4, 80, false));
		ussrTemplates.Insert(BuildTemplate("ussr_motor", "USSR Motorized Patrol", "USSR", 6, 85, true));
		ussrTemplates.Insert(BuildTemplate("ussr_armor", "USSR Armor Section", "USSR", 4, 90, true));
		m_mTemplatesByFaction.Set("USSR", ussrTemplates);
	}

	protected void EnsureFallbackFactionTemplates(string factionKey)
	{
		if (m_mTemplatesByFaction.Contains(factionKey))
			return;

		ref array<ref ARAL_ProfileTemplate> fallback = {};
		fallback.Insert(BuildTemplate(string.Format("%1_line", factionKey), string.Format("%1 Infantry Squad", factionKey), factionKey, 8, 60, false));
		fallback.Insert(BuildTemplate(string.Format("%1_motor", factionKey), string.Format("%1 Motorized Patrol", factionKey), factionKey, 6, 75, true));
		m_mTemplatesByFaction.Set(factionKey, fallback);
	}

	protected ARAL_ProfileTemplate BuildTemplate(string id, string displayName, string factionKey, int strength, int spawnPriority, bool vehicleProfile)
	{
		ARAL_ProfileTemplate profileTemplate = new ARAL_ProfileTemplate();
		profileTemplate.m_sId = id;
		profileTemplate.m_sDisplayName = displayName;
		profileTemplate.m_sFactionKey = factionKey;
		profileTemplate.m_iStrength = strength;
		profileTemplate.m_iSpawnPriority = spawnPriority;
		profileTemplate.m_bVehicleProfile = vehicleProfile;
		return profileTemplate;
	}
}
