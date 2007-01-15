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

#include "TableFAC.h"

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
/* Country code table according to ISO 3166 */

const struct elCountry TableCountryCode[LEN_TABLE_COUNTRY_CODE] = {
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
	{"io", "British Indian Ocean Ter."},
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
	{"cg", "Congo Democratic Rep."},
	{"cd", "Congo"},
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
	{"fk", "Falkland Islands"},
	{"fo", "Faroe Islands"},
	{"fj", "Fiji"},
	{"fi", "Finland"},
	{"fr", "France"},
	{"gf", "French Guiana"},
	{"pf", "French Polynesia"},
	{"tf", "French Southern Ter."},
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
	{"gg", "Guernsey"},
	{"gn", "Guinea"},
	{"gw", "Guinea-Bissau"},
	{"gy", "Guyana"},
	{"ht", "Haiti"},
	{"hm", "Heard Is. Mcdonald Is."},
	{"va", "Vatican City State"},
	{"hn", "Honduras"},
	{"hk", "Hong Kong"},
	{"hu", "Hungary"},
	{"is", "Iceland"},
	{"in", "India"},
	{"id", "Indonesia"},
	{"ir", "Iran"},
	{"iq", "Iraq"},
	{"im", "Isle of Man"},
	{"ie", "Ireland"},
	{"il", "Israel"},
	{"it", "Italy"},
	{"jm", "Jamaica"},
	{"jp", "Japan"},
	{"je", "Jersey"},
	{"jo", "Jordan"},
	{"kz", "Kazakhstan"},
	{"ke", "Kenya"},
	{"ki", "Kiribati"},
	{"kp", "Korea Democratic Rep."},
	{"kr", "Korea, Republic of"},
	{"kw", "Kuwait"},
	{"kg", "Kyrgyzstan"},
	{"la", "Lao People's Democratic Rep."},
	{"lv", "Latvia"},
	{"lb", "Lebanon"},
	{"ls", "Lesotho"},
	{"lr", "Liberia"},
	{"ly", "Libyan Arab Jamahiriya"},
	{"li", "Liechtenstein"},
	{"lt", "Lithuania"},
	{"lu", "Luxembourg"},
	{"mo", "Macao"},
	{"mk", "Macedonia"},
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
	{"fm", "Micronesia"},
	{"md", "Moldova"},
	{"mc", "Monaco"},
	{"mn", "Mongolia"},
	{"me", "Montenegro"},
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
	{"mp", "Northern Mariana Is."},
	{"no", "Norway"},
	{"om", "Oman"},
	{"pk", "Pakistan"},
	{"pw", "Palau"},
	{"ps", "Palestinian Territory"},
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
	{"rs", "Serbia"},
	{"sc", "Seychelles"},
	{"sl", "Sierra Leone"},
	{"sg", "Singapore"},
	{"sk", "Slovakia"},
	{"si", "Slovenia"},
	{"sb", "Solomon Islands"},
	{"so", "Somalia"},
	{"za", "South Africa"},
	{"gs", "South Georgia South Sandwich Is."},
	{"es", "Spain"},
	{"lk", "Sri Lanka"},
	{"sd", "Sudan"},
	{"sr", "Suriname"},
	{"sj", "Svalbard and Jan Mayen"},
	{"sz", "Swaziland"},
	{"se", "Sweden"},
	{"ch", "Switzerland"},
	{"sy", "Syrian Arab Republic"},
	{"tw", "Taiwan"},
	{"tj", "Tajikistan"},
	{"tz", "Tanzania"},
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
	{"um", "United States Is."},
	{"uy", "Uruguay"},
	{"uz", "Uzbekistan"},
	{"vu", "Vanuatu"},
	{"ve", "Venezuela"},
	{"vn", "Vietnam"},
	{"vg", "Virgin Islands, British"},
	{"vi", "Virgin Islands, U.S."},
	{"wf", "Wallis and Futuna"},
	{"eh", "Western Sahara"},
	{"ye", "Yemen"},
	{"zm", "Zambia"},
	{"zw", "Zimbabwe"}
};

/* Get country name from ISO 3166 A2 */

string GetISOCountryName(const string strA2)
{
	for (int i = 0; i < LEN_TABLE_COUNTRY_CODE; i++)
	{
		if (!strA2.compare(TableCountryCode[i].strcode))
			return TableCountryCode[i].strDesc;
	}

	return "";
}



/* CIRAF zones */

const string strTableCIRAFzones[LEN_TABLE_CIRAF_ZONES] = {
	"", /* 0 undefined */
	"Alaska", /* 1 */
	"west Canada", /* 2 */
	"central Canada - west", /* 3 */
	"central Canada - east, Baffin Island", /* 4 */
	"Greenland", /* 5 */
	"west USA", /* 6 */
	"central USA", /* 7 */
	"east USA", /* 8 */
	"east Canada", /* 9 */
	"Belize, Guatemala, Mexico", /* 10 */
	"Caribbean, central America",  /* 11 */
	"northwestern south America", /* 12 */
	"northeast Brazil", /* 13 */
	"southwestern south America", /* 14 */
	"southeast Brazil",  /* 15 */
	"south Argentina, south Chile, Falkland Islands", /* 16 */
	"Iceland", /* 17 */
	"Scandanavia", /* 18 */
	"west Russia northwest", /* 19 */
	"west Russia north", /* 20 */
	"central Russia northwest", /* 21 */
	"central Russia north", /* 22 */
	"central Russia east", /* 23 */
	"east Russia northwest", /* 24 */
	"east Russia north", /* 25 */
	"east Russia northeast", /* 26 */
	"northwest Europe", /* 27 */
	"central east south Europe", /* 28 */
	"Baltics and west Russia", /* 29 */
	"central Asia, west Russia southeast", /* 30 */
	"central Russia southwest, east Kazakhstan, east Kyrgyzstan", /* 31 */
	"central Russia south, west Mongolia", /* 32 */
	"central Russia southeast, east Mongolia", /* 33 */
	"east Russia southwest: Sakhalin, Sikhote Alin", /* 34 */
	"east Russia east: Kamchatka", /* 35 */
	"Azores, Canary Island, Madeira", /* 36 */
	"southwest Europe, northwest Africa", /* 37 */
	"Egypt, Libya", /* 38 */
	"Middle East", /* 39 */
	"Afghanistan, Iran", /* 40 */
	"Bangladesh, Bhutan, India, Nepal, Pakistan", /* 41 */
	"west China", /* 42 */
	"central China", /* 43 */
	"east China, Macao, Hong Kong, North Korea, South Korea, Taiwan", /* 44 */
	"Japan", /* 45 */
	"west Africa", /* 46 */
	"west Sudan", /* 47 */
	"Horn of Africa", /* 48 */
	"Kampuchea, Laos, Myanmar, Vietnam", /* 49 */
	"Philippines", /* 50 */
	"Malaysia, Papua New Guinea, west Indonesia", /* 51 */
	"Angola, Burundi, Congo, Gabon, Zaire", /* 52 */
	"Madagascar, Malawi, Mozambique, Seychelles, Zambia, Zimbabwe", /* 53 */
	"Malaysia, Singapore, west Indonesia", /* 54 */
	"northeast Australia", /* 55 */
	"Caledonia, Fiji/Vanuatu", /* 56 */
	"Botswana, Lesotho, Namibia, Swaziland, South African Republic", /* 57 */
	"west Australia", /* 58 */
	"southeast Australia", /* 59 */
	"New Zealand", /* 60 */
	"Hawaii", /* 61 */
	"Phoenix Islands, Samoa", /* 62 */
	"Cook Islands, Polynesia", /* 63 */
	"Guam/Palau, Saipan", /* 64 */
	"Kiribati, Marshall", /* 65 */
	"central Atlantic - south: Ascension, St. Helena", /* 66 */
	"Antarctica", /* 67 */
	"southwest Indian Ocean: Kerguelen", /* 68 */
	"Antarctica", /* 69 */
	"Antarctica", /* 70 */
	"Antarctica", /* 71 */
	"Antarctica", /* 72 */
	"Antarctica", /* 73 */
	"South Pole", /* 74 */
	"North Pole", /* 75 */
	"northeast Pacific", /* 76 */
	"central Pacific - northeast", /* 77 */
	"central Pacific - southeast", /* 78 */
	"central Indian Ocean", /* 79 */
	"northern Atlantic", /* 80 */
	"central Atlantic", /* 81 */
	"northwest Pacific", /* 82 */
	"south Pacific", /* 83 */
	"south Atlantic", /* 84 */
	"southeast Indian Ocean" /* 85 */
};
