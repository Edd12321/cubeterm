/* Which method to solve with */
const std::string default_method = "ZZ";

/* By default, these are the same bindings as csTimer VC */
const std::map<char, std::string> sim_keys = {
	{ '1', "S'" }, { '2', "E " },                               { '5', "R'" }, { '6', "M " },                { '9', "E'" }, { '0', "S " },
	{ 'q', "z'" }, { 'w', "B " }, { 'e', "L'" }, { 'r', "l'" }, { 't', "x " }, { 'y', "x " }, { 'u', "r " }, { 'i', "R " }, { 'o', "B'" }, { 'p', "z " },
	{ 'a', "y'" }, { 's', "D " }, { 'd', "L " }, { 'f', "U'" }, { 'g', "F'" }, { 'h', "F " }, { 'j', "U " }, { 'k', "R'" }, { 'l', "D'" }, { ';', "y " },
	{ 'z', "d " }, { 'x', "M'" }, { 'c', "u'" }, { 'v', "l " }, { 'b', "x'" }, { 'n', "x'" }, { 'm', "r'" }, { ',', "u " }, { '.', "M'" }, { '/', "d'" },
};
