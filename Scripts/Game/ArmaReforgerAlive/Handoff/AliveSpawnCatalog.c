[ComponentEditorProps(category: "Arma Reforger Alive/Handoff", description: "Template-to-prefab mapping used by the physical projection handoff layer.")]
class AliveSpawnCatalogComponentClass : ScriptComponentClass
{
}

interface IAliveSpawnCatalog
{
	bool ResolveRootPrefab(string templateId, out ResourceName rootPrefab);
}

class AliveSpawnCatalogComponent : ScriptComponent, IAliveSpawnCatalog
{
	[Attribute("1", UIWidgets.CheckBox, "Automatically search loaded faction entity catalogs when a template prefab is not explicitly configured.", category: "Runtime")]
	protected bool m_bAutoDiscoverVanillaPrefabs;

	[Attribute("", UIWidgets.EditBox, "Vanilla US rifle profile root prefab.", category: "US")]
	protected ResourceName m_sUsRifleRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla US weapons profile root prefab.", category: "US")]
	protected ResourceName m_sUsWeaponsRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla US recon profile root prefab.", category: "US")]
	protected ResourceName m_sUsReconRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla US motorized profile root prefab.", category: "US")]
	protected ResourceName m_sUsMotorRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla US armor profile root prefab.", category: "US")]
	protected ResourceName m_sUsArmorRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla USSR rifle profile root prefab.", category: "USSR")]
	protected ResourceName m_sUssrRifleRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla USSR weapons profile root prefab.", category: "USSR")]
	protected ResourceName m_sUssrWeaponsRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla USSR recon profile root prefab.", category: "USSR")]
	protected ResourceName m_sUssrReconRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla USSR motorized profile root prefab.", category: "USSR")]
	protected ResourceName m_sUssrMotorRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Vanilla USSR armor profile root prefab.", category: "USSR")]
	protected ResourceName m_sUssrArmorRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Fallback infantry root prefab for unresolved template ids.", category: "Fallback")]
	protected ResourceName m_sDefaultInfantryRootPrefab;

	[Attribute("", UIWidgets.EditBox, "Fallback vehicle root prefab for unresolved template ids.", category: "Fallback")]
	protected ResourceName m_sDefaultVehicleRootPrefab;

	protected ref AliveVanillaPrefabDiscoveryService m_DiscoveryService = new AliveVanillaPrefabDiscoveryService();

	bool ResolveRootPrefab(string templateId, out ResourceName rootPrefab)
	{
		rootPrefab = "";

		switch (templateId)
		{
			case "us_rifle":
				rootPrefab = m_sUsRifleRootPrefab;
				break;
			case "us_weapons":
				rootPrefab = m_sUsWeaponsRootPrefab;
				break;
			case "us_recon":
				rootPrefab = m_sUsReconRootPrefab;
				break;
			case "us_motor":
				rootPrefab = m_sUsMotorRootPrefab;
				break;
			case "us_armor":
				rootPrefab = m_sUsArmorRootPrefab;
				break;
			case "ussr_rifle":
				rootPrefab = m_sUssrRifleRootPrefab;
				break;
			case "ussr_weapons":
				rootPrefab = m_sUssrWeaponsRootPrefab;
				break;
			case "ussr_recon":
				rootPrefab = m_sUssrReconRootPrefab;
				break;
			case "ussr_motor":
				rootPrefab = m_sUssrMotorRootPrefab;
				break;
			case "ussr_armor":
				rootPrefab = m_sUssrArmorRootPrefab;
				break;
		}

		if (rootPrefab == "")
		{
			bool isVehicleTemplate = templateId.Contains("motor") || templateId.Contains("armor");
			if (m_bAutoDiscoverVanillaPrefabs && m_DiscoveryService.ResolveTemplatePrefab(templateId, isVehicleTemplate, rootPrefab))
				return true;

			if (isVehicleTemplate)
				rootPrefab = m_sDefaultVehicleRootPrefab;
			else
				rootPrefab = m_sDefaultInfantryRootPrefab;
		}

		return rootPrefab != "";
	}
}
