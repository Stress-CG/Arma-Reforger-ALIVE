class AliveVanillaPrefabDiscoveryService : Managed
{
	protected ref map<string, ResourceName> m_mResolvedTemplatePrefabs = new map<string, ResourceName>();

	bool ResolveTemplatePrefab(string templateId, bool vehicleProfile, out ResourceName prefab)
	{
		prefab = "";

		string cacheKey = templateId;
		ResourceName cachedPrefab = m_mResolvedTemplatePrefabs.Get(cacheKey);
		if (cachedPrefab != "")
		{
			prefab = cachedPrefab;
			return true;
		}

		string factionKey = ResolveFactionKeyForTemplate(templateId);
		if (factionKey == "")
			return false;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return false;

		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		if (!faction)
			return false;

		array<SCR_EntityCatalog> catalogs = {};
		faction.GetAllFactionEntityCatalogs(catalogs);

		float bestScore = -9999.0;
		foreach (SCR_EntityCatalog catalog : catalogs)
		{
			if (!catalog)
				continue;

			array<SCR_EntityCatalogEntry> entries = {};
			catalog.GetFullFilteredEntityList(entries);
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				if (!entry)
					continue;

				ResourceName candidatePrefab = entry.GetPrefab();
				if (candidatePrefab == "")
					continue;

				float score = ScoreCandidate(templateId, vehicleProfile, candidatePrefab);
				if (score <= bestScore)
					continue;

				bestScore = score;
				prefab = candidatePrefab;
			}
		}

		if (prefab == "" || bestScore < 25.0)
			return false;

		m_mResolvedTemplatePrefabs.Set(cacheKey, prefab);
		ARAL_Log.Info(string.Format("Auto-discovered prefab '%1' for template '%2'.", prefab, templateId));
		return true;
	}

	protected string ResolveFactionKeyForTemplate(string templateId)
	{
		int separator = templateId.IndexOf("_");
		if (separator <= 0)
			return "";

		string prefix = templateId.Substring(0, separator);
		prefix.ToUpper();
		return prefix;
	}

	protected float ScoreCandidate(string templateId, bool vehicleProfile, ResourceName candidatePrefab)
	{
		string candidate = candidatePrefab;
		candidate.ToLower();

		float score = 0;
		if (vehicleProfile)
		{
			if (candidate.Contains("/vehicles/"))
				score += 40;
			if (ContainsAny(candidate, BuildKeywordArray("btr bmp brdm humvee uaz m151 truck car jeep apc armor")))
				score += 25;
			if (candidate.Contains("/characters/"))
				score -= 40;
		}
		else
		{
			if (candidate.Contains("/characters/"))
				score += 40;
			if (ContainsAny(candidate, BuildKeywordArray("group team squad patrol")))
				score += 20;
			if (candidate.Contains("/vehicles/"))
				score -= 40;
		}

		if (templateId.Contains("rifle"))
			score += ScoreKeywordSet(candidate, BuildKeywordArray("rifle squad patrol fireteam"), BuildKeywordArray("recon sniper crew vehicle armor"));
		else if (templateId.Contains("weapons"))
			score += ScoreKeywordSet(candidate, BuildKeywordArray("weapon gunner support mg at rpg"), BuildKeywordArray("recon sniper car truck"));
		else if (templateId.Contains("recon"))
			score += ScoreKeywordSet(candidate, BuildKeywordArray("recon scout sniper special specops"), BuildKeywordArray("armor truck apc"));
		else if (templateId.Contains("motor"))
			score += ScoreKeywordSet(candidate, BuildKeywordArray("car truck jeep uaz humvee m151 motor"), BuildKeywordArray("armor bmp btr sniper"));
		else if (templateId.Contains("armor"))
			score += ScoreKeywordSet(candidate, BuildKeywordArray("armor apc btr bmp brdm tank"), BuildKeywordArray("scout sniper truck jeep"));

		if (ContainsAny(candidate, BuildKeywordArray("empty composition editor test civilian")))
			score -= 35;

		return score;
	}

	protected float ScoreKeywordSet(string candidate, array<string> positive, array<string> negative)
	{
		float score = 0;
		foreach (string keyword : positive)
		{
			if (candidate.Contains(keyword))
				score += 8;
		}

		foreach (string negativeKeyword : negative)
		{
			if (candidate.Contains(negativeKeyword))
				score -= 10;
		}

		return score;
	}

	protected bool ContainsAny(string candidate, array<string> keywords)
	{
		foreach (string keyword : keywords)
		{
			if (candidate.Contains(keyword))
				return true;
		}

		return false;
	}

	protected array<string> BuildKeywordArray(string spaceSeparatedKeywords)
	{
		array<string> keywords = {};
		spaceSeparatedKeywords.Split(" ", keywords, true);
		return keywords;
	}
}
