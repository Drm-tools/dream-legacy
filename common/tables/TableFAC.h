/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Tables for FAC
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#if !defined(TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_)
#define TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_

#include <string>
#include "../GlobalDefinitions.h"


/* Definitions ****************************************************************/
/* ETSI ES201980V2.1.1: page 115, 7.5.3: ...FAC shall use 4-QAM mapping. A
   fixed code rate shall be applied...R_all=0.6...
   6 tailbits are used for the encoder to get in zero state ->
   65 [number of cells] * 2 [4-QAM] * 0.6 [code-rate] - 6 [tailbits] = 72 */
#define NUM_FAC_BITS_PER_BLOCK			72

/* iTableNumOfServices[a][b]
   a: Number of audio services
   b: Number of data services 
   (6.3.4) */
const int iTableNumOfServices[5][5] = {
	/* -> Data */
	{-1,  1,  2,  3, 15},
	{ 4,  5,  6,  7, -1},
	{ 8,  9, 10, -1, -1},
	{12, 13, -1, -1, -1},
	{ 0, -1, -1, -1, -1}
};

/* Language code */
#define LEN_TABLE_LANGUAGE_CODE			16

const string strTableLanguageCode[LEN_TABLE_LANGUAGE_CODE] = {
	"No language specified", 
	"Arabic", 
	"Bengali", 
	"Chinese (Mandarin)", 
	"Dutch", 
	"English", 
	"French", 
	"German", 
	"Hindi", 
	"Japanese", 
	"Javanese", 
	"Korean", 
	"Portuguese", 
	"Russian", 
	"Spanish", 
	"Other language"
};

/* Programme Type codes */
#define LEN_TABLE_PROG_TYPE_CODE_TOT	32
#define LEN_TABLE_PROG_TYPE_CODE		30

const string strTableProgTypCod[LEN_TABLE_PROG_TYPE_CODE_TOT] = {
	"No programme type",
	"News",
	"Current Affairs",
	"Information",
	"Sport",
	"Education",
	"Drama",
	"Culture",
	"Science",
	"Varied",
	"Pop Music",
	"Rock Music",
	"Easy Listening Music",
	"Light Classical",
	"Serious Classical",
	"Other Music",
	"Weather/meteorology",
	"Finance/Business",
	"Children's programmes",
	"Social Affairs",
	"Religion",
	"Phone In",
	"Travel",
	"Leisure",
	"Jazz Music",
	"Country Music",
	"National Music",
	"Oldies Music",
	"Folk Music",
	"Documentary",
	"Not used",
	"Not used"
};


// TODO: the following table can be used for country code decoding
// FIXME: if the following code is activated, the compiling under Linux takes forever...?
/* Country code table according to ISO 3166 */
/*
#define LEN_TABLE_CNTRY_CODE			240

const string strTableCntryCod[LEN_TABLE_CNTRY_CODE][2] = {
	{"af", "Afghanistan"},
	{"ax", "Aland Islands"},
	{"al", "Albania"},
	{"dz", "Algeria"},
	{"as", "American Samoa"},
	{"ad", "Andorra"},
	{"ao", "Angola"},
	{"ai", "Anguilla"},
	{"aq", "Antarctica"},
	{"ag", "Antigua and barbuda"},
	{"ar", "Argentina"},
	{"am", "Armenia"},
	{"aw", "Aruba"},
	{"au", "Australia"},
	{"at", "Austria"},
	{"az", "Azerbaijan"},
	{"bs", "Bahamas"},
	{"bh", "Bahrain"},
	{"bd", "Bangladesh"},
	{"bb", "Barbados"},
	{"by", "Belarus"},
	{"be", "Belgium"},
	{"bz", "Belize"},
	{"bj", "Benin"},
	{"bm", "Bermuda"},
	{"bt", "Bhutan"},
	{"bo", "Bolivia"},
	{"ba", "Bosnia and Herzegovina"},
	{"bw", "Botswana"},
	{"bv", "Bouvet Island"},
	{"br", "Brazil"},
	{"io", "British Indian Ocean Territory"},
	{"bn", "Brunei Darussalam"},
	{"bg", "Bulgaria"},
	{"bf", "Burkina Faso"},
	{"bi", "Burundi"},
	{"kh", "Cambodia"},
	{"cm", "Cameroon"},
	{"ca", "Canada"},
	{"cv", "Cape Verde"},
	{"ky", "Cayman Islands"},
	{"cf", "Central African Republic"},
	{"td", "Chad"},
	{"cl", "Chile"},
	{"cn", "China"},
	{"cx", "Christmas Island"},
	{"cc", "Cocos (Keeling) Islands"},
	{"co", "Colombia"},
	{"km", "Comoros"},
	{"cg", "Congo"},
	{"cd", "Congo, the Democratic Republic of the"},
	{"ck", "Cook Islands"},
	{"cr", "Costa Rica"},
	{"ci", "Côte d'Ivoire"},
	{"hr", "Croatia"},
	{"cu", "Cuba"},
	{"cy", "Cyprus"},
	{"cz", "Czech Republic"},
	{"dk", "Denmark"},
	{"dj", "Djibouti"},
	{"dm", "Dominica"},
	{"do", "Dominican Republic"},
	{"ec", "Ecuador"},
	{"eg", "Egypt"},
	{"sv", "El Salvador"},
	{"gq", "Equatorial Guinea"},
	{"er", "Eritrea"},
	{"ee", "Estonia"},
	{"et", "Ethiopia"},
	{"fk", "Falkland Islands (Malvinas)"},
	{"fo", "Faroe Islands"},
	{"fj", "Fiji"},
	{"fi", "Finland"},
	{"fr", "France"},
	{"gf", "French Guiana"},
	{"pf", "French Polynesia"},
	{"tf", "French Southern Territories"},
	{"ga", "Gabon"},
	{"gm", "Gambia"},
	{"ge", "Georgia"},
	{"de", "Germany"},
	{"gh", "Ghana"},
	{"gi", "Gibraltar"},
	{"gr", "Greece"},
	{"gl", "Greenland"},
	{"gd", "Grenada"},
	{"gp", "Guadeloupe"},
	{"gu", "Guam"},
	{"gt", "Guatemala"},
	{"gn", "Guinea"},
	{"gw", "Guinea-Bissau"},
	{"gy", "Guyana"},
	{"ht", "Haiti"},
	{"hm", "Heard Island and Mcdonald Islands"},
	{"va", "Holy see (Vatican City State)"},
	{"hn", "Honduras"},
	{"hk", "Hong Kong"},
	{"hu", "Hungary"},
	{"is", "Iceland"},
	{"in", "India"},
	{"id", "Indonesia"},
	{"ir", "Iran, Islamic Republic of"},
	{"iq", "Iraq"},
	{"ie", "Ireland"},
	{"il", "Israel"},
	{"it", "Italy"},
	{"jm", "Jamaica"},
	{"jp", "Japan"},
	{"jo", "Jordan"},
	{"kz", "Kazakhstan"},
	{"ke", "Kenya"},
	{"ki", "Kiribati"},
	{"kp", "Korea, Democratic People's Republic of"},
	{"kr", "Korea, Republic of"},
	{"kw", "Kuwait"},
	{"kg", "Kyrgyzstan"},
	{"la", "Lao People's Democratic Republic"},
	{"lv", "Latvia"},
	{"lb", "Lebanon"},
	{"ls", "Lesotho"},
	{"lr", "Liberia"},
	{"ly", "Libyan Arab Jamahiriya"},
	{"li", "Liechtenstein"},
	{"lt", "Lithuania"},
	{"lu", "Luxembourg"},
	{"mo", "Macao"},
	{"mk", "Macedonia, the Former Yugoslav Republic of"},
	{"mg", "Madagascar"},
	{"mw", "Malawi"},
	{"my", "Malaysia"},
	{"mv", "Maldives"},
	{"ml", "Mali"},
	{"mt", "Malta"},
	{"mh", "Marshall Islands"},
	{"mq", "Martinique"},
	{"mr", "Mauritania"},
	{"mu", "Mauritius"},
	{"yt", "Mayotte"},
	{"mx", "Mexico"},
	{"fm", "Micronesia, Federated States of"},
	{"md", "Moldova, Republic of"},
	{"mc", "Monaco"},
	{"mn", "Mongolia"},
	{"ms", "Montserrat"},
	{"ma", "Morocco"},
	{"mz", "Mozambique"},
	{"mm", "Myanmar"},
	{"na", "Namibia"},
	{"nr", "Nauru"},
	{"np", "Nepal"},
	{"nl", "Netherlands"},
	{"an", "Netherlands Antilles"},
	{"nc", "New Caledonia"},
	{"nz", "New Zealand"},
	{"ni", "Nicaragua"},
	{"ne", "Niger"},
	{"ng", "Nigeria"},
	{"nu", "Niue"},
	{"nf", "Norfolk Island"},
	{"mp", "Northern Mariana Islands"},
	{"no", "Norway"},
	{"om", "Oman"},
	{"pk", "Pakistan"},
	{"pw", "Palau"},
	{"ps", "Palestinian Territory, Occupied"},
	{"pa", "Panama"},
	{"pg", "Papua New Guinea"},
	{"py", "Paraguay"},
	{"pe", "Peru"},
	{"ph", "Philippines"},
	{"pn", "Pitcairn"},
	{"pl", "Poland"},
	{"pt", "Portugal"},
	{"pr", "Puerto Rico"},
	{"qa", "Qatar"},
	{"re", "Réunion"},
	{"ro", "Romania"},
	{"ru", "Russian Federation"},
	{"rw", "Rwanda"},
	{"sh", "Saint Helena"},
	{"kn", "Saint Kitts and Nevis"},
	{"lc", "Saint Lucia"},
	{"pm", "Saint Pierre and Miquelon"},
	{"vc", "Saint Vincent and the Grenadines"},
	{"ws", "Samoa"},
	{"sm", "San Marino"},
	{"st", "Sao Tome and Principe"},
	{"sa", "Saudi arabia"},
	{"sn", "Senegal"},
	{"cs", "Serbia and Montenegro"},
	{"sc", "Seychelles"},
	{"sl", "Sierra Leone"},
	{"sg", "Singapore"},
	{"sk", "Slovakia"},
	{"si", "Slovenia"},
	{"sb", "Solomon Islands"},
	{"so", "Somalia"},
	{"za", "South Africa"},
	{"gs", "South Georgia and the South Sandwich Islands"},
	{"es", "Spain"},
	{"lk", "Sri Lanka"},
	{"sd", "Sudan"},
	{"sr", "Suriname"},
	{"sj", "Svalbard and Jan Mayen"},
	{"sz", "Swaziland"},
	{"se", "Sweden"},
	{"ch", "Switzerland"},
	{"sy", "Syrian Arab Republic"},
	{"tw", "Taiwan, Province of China"},
	{"tj", "Tajikistan"},
	{"tz", "Tanzania, United Republic of"},
	{"th", "Thailand"},
	{"tl", "Timor-Leste"},
	{"tg", "Togo"},
	{"tk", "Tokelau"},
	{"to", "Tonga"},
	{"tt", "Trinidad and Tobago"},
	{"tn", "Tunisia"},
	{"tr", "Turkey"},
	{"tm", "Turkmenistan"},
	{"tc", "Turks and Caicos Islands"},
	{"tv", "Tuvalu"},
	{"ug", "Uganda"},
	{"ua", "Ukraine"},
	{"ae", "United Arab Emirates"},
	{"gb", "United Kingdom"},
	{"us", "United States"},
	{"um", "United States Minor Outlying Islands"},
	{"uy", "Uruguay"},
	{"uz", "Uzbekistan"},
	{"vu", "Vanuatu"},
	{"ve", "Venezuela"},
	{"vn", "Viet nam"},
	{"vg", "Virgin Islands, British"},
	{"vi", "Virgin Islands, U.S."},
	{"wf", "Wallis and Futuna"},
	{"eh", "Western Sahara"},
	{"ye", "Yemen"},
	{"zm", "Zambia"},
	{"zw", "Zimbabwe"}
};
*/

/* Get country name from ISO 3166 A2 */
/*
static string GetName(const string strA2)
{
	for (int i = 0; i < LEN_TABLE_CNTRY_CODE; i++)
	{
		if (!strA2.compare(strTableCntryCod[i][0]))
			return strTableCntryCod[i][1];
	}

	return "";
}
*/


/* CIRAF zones */
#define LEN_TABLE_CIRAF_ZONES			86

const string strTableCIRAFzones[LEN_TABLE_CIRAF_ZONES] = {
	"", /* undefined */
	"Alaska",
	"western Canada",
	"central Canada - west",
	"central Canada - east, Baffin Island",
	"Greenland",
	"western USA",
	"central USA",
	"eastern USA",
	"eastern Canada",
	"Belize, Guatemala, Mexico",

	"Caribbean, central America",
//[11] = "Antilles, Costa Rica, Cuba, Dominican Republic, El Salvador, Haiti, Honduras, Jamaica, Nicaragua, Panama, Puerto Rico, West Indies",

	"northwestern south America",
//[12] = "Colombia, Ecuador, French Guiana, Guyana, northern Bolivia, northwestern Brazil, Peru, Surinam, Venezuela",

	"northeastern Brazil",
	"northern Argentina, northern Chile, Paraguay, southern Bolivia, Uruguay",
	"southeastern Brazil",
	"southern Argentina, southern Chile, Falkland Islands",
	"Iceland",

	"Scandanavia",
//[18] = "Denmark, Finland, Norway, Sweden",

	"western Russia northwest",
	"western Russia north",
	"central Russia northwest",
	"central Russia north",
	"central Russia east",
	"eastern Russia northwest",
	"eastern Russia north",
	"eastern Russia northeast",

	"UK, France, Benelux",
//[27] = "Belgium, France, Great Britain, Ireland, Monaco, Netherlands",

	"central and eastern Europe",
//[28] = "Albania, Austria, Bosnia and Herzegovina, Bulgaria, Croatia, Czech Republic, Germany, Greece, Hungary, Italy, Luxembourg, Macedonia, Malta, Poland, Romania, Slovakia, Slovenia, Switzerland, Vatican, Yugoslavia",

	"Baltics and western Russia",
//[29] = "Armenia, Azerbaijan, Belarus, Estonia, Georgia, Latvia, Lithuania, Moldova, Ukraine, western Russia",

	"Tajikistan, Turkmenistan, Uzbekistan, western Kazakhstan, western Kyrgyzstan, western Russia southeast",
	"central Russia southwest, eastern Kazakhstan, eastern Kyrgyzstan",
	"central Russia south, western Mongolia",
	"central Russia southeast, eastern Mongolia",
	"eastern Russia southwest: Sakhalin, Sikhote Alin",
	"eastern Russia east: Kamchatka",
	"Azores, Canary Island, Madeira",
	"Algeria, Andorra, Morocco, Portugal, Spain, Tunisia",
	"Egypt, Libya",

	"Turkey, Iraq, Syria, Lebanon, Israel, Jordan, Arabian Pen.",
//[39] = "Bahrain, Cyprus, Iraq, Israel, Jordan, Kuwait, Lebanon, Oman, Qatar, Saudi Arabia, Syria, Turkey, United Arab Emirates, Yemen",

	"Afghanistan, Iran",
	"Bangladesh, Bhutan, India, Nepal, Pakistan",
	"western China",
	"central China",
	"eastern China, Macao, Hong Kong, North Korea, South Korea, Taiwan",
	"Japan",

	"west Africa",
//[46] = "Benin, Burkina Faso, Cape Verde, C?e d'Ivoire, Gambia, Ghana, Guinea, Guinea-Bissau, Liberia, Mali, Mauritania, Niger, Nigeria, Senegal, Sierra Leone, Togo, Cameroon, Chad, Central African Republic, Equatorial Guinea, Gabon, S? Tom",

	"western Sudan",
	"Burundi, Eritrea, Ethiopia, eastern Sudan, Kenya, Somalia, Uganda",
	"Kampuchea, Laos, Myanmar, Vietnam",
	"Philippines",
	"Malaysia, Papua New Guinea, western Indonesia",
	"Angola, Burundi, Congo, Gabon, Zaire",
	"Madagascar, Malawi, Mozambique, Seychelles, Zambia, Zimbabwe",
	"Malaysia, Singapore, western Indonesia",
	"northeastern Australia",
	"Caledonia, Fiji/Vanuatu",
	"Botswana, Lesotho, Namibia, Swaziland, South African Republic",
	"western Australia",
	"southeastern Australia",
	"New Zealand",
	"Hawaii",
	"Phoenix Islands, Samoa",
	"Cook Islands, Polynesia",
	"Guam/Palau, Saipan",
	"Kiribati, Marshall",
	"central Atlantic - south: Ascension, St. Helena",
	"Antarctica",
	"southwestern Indian Ocean: Kerguelen",
	"Antarctica",
	"Antarctica",
	"Antarctica",
	"Antarctica",
	"Antarctica",
	"South Pole",
	"North Pole",
	"northeastern Pacific",
	"central Pacific - northeast",
	"central Pacific - southeast",
	"central Indian Ocean",
	"northern Atlantic",
	"central Atlantic",
	"northwestern Pacific",
	"southern Pacific",
	"southern Atlantic",
	"southeastern Indian Ocean"
};


#endif // !defined(TABLE_FAC_H__3B0_CA63_4344_BGDEB2B_23E7912__INCLUDED_)
