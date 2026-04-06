[ComponentEditorProps(category: "Arma Reforger Alive/World", description: "Author override for a scanned campaign objective.", color: "0 0.7 0.4 1", visible: true)]
class ARAL_ObjectiveSeedComponentClass : ScriptComponentClass
{
}

class ARAL_ObjectiveSeedComponent : ScriptComponent
{
	[Attribute("Objective", UIWidgets.EditBox, "Display name for this objective.", category: "Objective")]
	protected string m_sObjectiveName;

	[Attribute("250", UIWidgets.EditBox, "Capture radius for this objective.", params: "50 2000 10", category: "Objective")]
	protected float m_fRadius;

	[Attribute("75", UIWidgets.Slider, "Relative planner priority.", params: "0 100 1", category: "Objective")]
	protected int m_iPriority;

	[Attribute("6", UIWidgets.ComboBox, "Objective type.", enums: ParamEnumArray.FromEnum(ARAL_EObjectiveKind), category: "Objective")]
	protected ARAL_EObjectiveKind m_eKind;

	[Attribute("", UIWidgets.EditBox, "Optional faction key to start owning this objective.", category: "Ownership")]
	protected string m_sStartingOwnerFactionKey;

	ARAL_Objective BuildObjective(IEntity owner, int ordinal)
	{
		ARAL_Objective objective = new ARAL_Objective();
		objective.m_sName = m_sObjectiveName;
		objective.m_vPosition = owner.GetOrigin();
		objective.m_fRadius = m_fRadius;
		objective.m_iPriority = m_iPriority;
		objective.m_eKind = m_eKind;
		objective.m_sOwnerFactionKey = m_sStartingOwnerFactionKey;
		objective.m_bAuthorOverride = true;
		objective.m_sId = ARAL_IdUtility.MakeStableId("seed", m_sObjectiveName, objective.m_vPosition, ordinal);
		return objective;
	}
}
