/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Tomi Manninen
 *
 * 5/15/2005 Andrea Russo
 *	- added preview
 * 5/25/2005 Andrea Russo
 *	- added "days" column in stations list view
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

#include "StationsDlg.h"
#include <qdatetime.h>
#include <qftp.h>
#if QT_VERSION >= 0x030000
# include <qhttp.h>
#endif

/* Implementation *************************************************************/
void CDRMSchedule::ReadStatTabFromFile(const ESchedMode eNewSchM)
{
	const int	iMaxLenName = 256;
	char		cName[iMaxLenName];
	FILE*		pFile = NULL;

	/* Save new mode */
	SetSchedMode(eNewSchM);

	/* Open file and init table for stations */
	StationsTable.clear();

	switch (eNewSchM)
	{
	case SM_DRM:
		pFile = fopen(DRMSCHEDULE_INI_FILE_NAME, "r");
		break;

	case SM_ANALOG:
		pFile = fopen(AMSCHEDULE_INI_FILE_NAME, "r");
		break;
	}

	/* Check if opening of file was successful */
	if (pFile == 0)
		return;

	fgets(cName, iMaxLenName, pFile); /* Remove "[DRMSchedule]" */
	if(cName[0] == '[')
		ReadIniFile(pFile);
	else
		ReadCSVFile(pFile);
	fclose(pFile);
}

void CDRMSchedule::ReadIniFile(FILE* pFile)
{
	const int	iMaxLenName = 256;
	char		cName[iMaxLenName];
	int			iFileStat;
	_BOOLEAN	bReadOK = TRUE;

	fgets(cName, iMaxLenName, pFile); /* Remove "[DRMSchedule]" */
	do
	{
		CStationsItem StationsItem;

		/* Start stop time */
		int iStartTime, iStopTime;
		iFileStat = fscanf(pFile, "StartStopTimeUTC=%04d-%04d\n",
			&iStartTime, &iStopTime);

		if (iFileStat != 2)
			bReadOK = FALSE;
		else
		{
			StationsItem.SetStartTimeNum(iStartTime);
			StationsItem.SetStopTimeNum(iStopTime);
		}

		/* Days */
		/* Init days with the "irregular" marker in case no valid string could
		   be read */
		string strNewDaysFlags = FLAG_STR_IRREGULAR_TRANSM;

		iFileStat = fscanf(pFile, "Days[SMTWTFS]=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
		{
			/* Check for length of input string (must be 7) */
			const string strTMP = cName;
			if (strTMP.length() == 7)
				strNewDaysFlags = strTMP;
		}

		/* Frequency */
		iFileStat = fscanf(pFile, "Frequency=%d\n", &StationsItem.iFreq);
		if (iFileStat != 1)
			bReadOK = FALSE;

		/* Target */
		iFileStat = fscanf(pFile, "Target=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strTarget = cName;

		/* Power */
		iFileStat = fscanf(pFile, "Power=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.rPower = QString(cName).toFloat();

		/* Name of the station */
		iFileStat = fscanf(pFile, "Programme=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strName = cName;

		/* Language */
		iFileStat = fscanf(pFile, "Language=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strLanguage = cName;

		/* Site */
		iFileStat = fscanf(pFile, "Site=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strSite = cName;

		/* Country */
		iFileStat = fscanf(pFile, "Country=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strCountry = cName;

		iFileStat = fscanf(pFile, "\n");

		/* Check for error before applying data */
		if (bReadOK == TRUE)
		{
			/* Set "days flag string" and generate strings for displaying active
			   days */
			StationsItem.SetDaysFlagString(strNewDaysFlags);

			/* Add new item in table */
			StationsTable.push_back(StationsItem);
		}
	} while (!((iFileStat == EOF) || (bReadOK == FALSE)));
}

static struct { char *code; char *lang; } langs[] = {
	{ "-TS", "Time Signal Station" },
	{ "A", "Arabic" },
	{ "AB", "Abkhaz" },
	{ "AC", "Aceh" },
	{ "ACH", "Achang" },
	{ "AD", "Adygea" },
	{ "ADI", "Adi" },
	{ "AF", "Afrikaans" },
	{ "AFA", "Afar" },
	{ "AFG", "Pashto and Dari" },
	{ "AH", "Amharic" },
	{ "AK", "Akha" },
	{ "AL", "Albanian" },
	{ "ALG", "Algerian" },
	{ "AM", "Amoy" },
	{ "Ang", "Angelus programme of Vaticane Radio" },
	{ "AR", "Armenian" },
	{ "ARO", "Aromanian" },
	{ "ARU", "Arunachal" },
	{ "ASS", "Assamese" },
	{ "ASY", "Assyrian" },
	{ "ATS", "Atsi" },
	{ "Aud", "Papal Audience" },
	{ "AV", "Avar" },
	{ "AW", "Awadhi" },
	{ "AY", "Aymara" },
	{ "AZ", "Azeri" },
	{ "B", "Brazilian" },
	{ "BAD", "Badaga" },
	{ "BAG", "Bagri" },
	{ "BAJ", "Bajau" },
	{ "BAK", "Baku" },
	{ "BAL", "Balinese" },
	{ "BAN", "Banjar" },
	{ "BAO", "Baoule" },
	{ "BAR", "Bari" },
	{ "BAS", "Bashkir" },
	{ "BAY", "Bayash Romani" },
	{ "BB", "Braj Bhasa" },
	{ "BC", "Baluchi" },
	{ "BE", "Bengali" },
	{ "BEM", "Bemba" },
	{ "BH", "Bhili" },
	{ "BHN", "Bahnar" },
	{ "BI", "Bile" },
	{ "BID", "Bidayuh" },
	{ "BIS", "Bisaya" },
	{ "BJ", "Bhojpuri" },
	{ "BK", "Balkarian" },
	{ "BLK", "Balkan Romani" },
	{ "BLT", "Balti" },
	{ "BM", "Bambara" },
	{ "BNA", "Borana Oromo" },
	{ "BNG", "Bangala" },
	{ "BNJ", "Banjari" },
	{ "BOR", "Boro" },
	{ "BOS", "Bosnian" },
	{ "BR", "Burmese" },
	{ "BRA", "Brahui" },
	{ "BRI", "Brij" },
	{ "Bru", "Bru" },
	{ "BSL", "Bislama" },
	{ "BT", "Black Tai" },
	{ "BTK", "Batak-Toba" },
	{ "BU", "Bulgarian" },
	{ "BUG", "Bugis" },
	{ "BUK", "Bukharian" },
	{ "BUN", "Bundeli" },
	{ "BUR", "Buryat" },
	{ "BY", "Byelorussian" },
	{ "C", "Chinese" },
	{ "CA", "Cantonese" },
	{ "CC", "Chaochow" },
	{ "CD", "Chowdary" },
	{ "CEB", "Cebuano" },
	{ "CH", "Chin" },
	{ "C-A", "Chin-Asho" },
	{ "C-D", "Chin-Daai" },
	{ "CHE", "Chechen" },
	{ "CHG", "Chattisgarhi" },
	{ "CHI", "Chitrali" },
	{ "C-K", "Chin-Khumi" },
	{ "C-O", "Chin-Thado" },
	{ "CHR", "Chrau" },
	{ "CHT", "Chatrali" },
	{ "CHU", "Chuwabu" },
	{ "C-T", "Chin-Tidim" },
	{ "C-Z", "Chin-Zomin" },
	{ "CI", "Circassic" },
	{ "CKM", "Chakma" },
	{ "CKW", "Chokwe" },
	{ "COF", "Cofan" },
	{ "COK", "Cook Islands Maori" },
	{ "CR", "Creole" },
	{ "CRU", "Chru" },
	{ "CT", "Catalan" },
	{ "CV", "Chuvash" },
	{ "CW", "Chewa" },
	{ "CZ", "Czech" },
	{ "D", "German" },
	{ "D-P", "Lower German" },
	{ "DA", "Danish" },
	{ "DAO", "Dao" },
	{ "DD", "Dhodiya" },
	{ "DEG", "Degar" },
	{ "DH", "Dhivehi" },
	{ "DI", "Dinka" },
	{ "DIA", "Dial Arabic" },
	{ "DN", "Damara-Nama service in Namibia" },
	{ "DO", "Dogri-Kangri" },
	{ "DR", "Dari" },
	{ "DU", "Dusun" },
	{ "DUN", "Dungan" },
	{ "DY", "Dyula" },
	{ "DZ", "Dzonkha" },
	{ "E", "English" },
	{ "EC", "Eastern Cham" },
	{ "EDI", "English, German, Italian" },
	{ "E-C", "Col.English" },
	{ "EGY", "Egyptian Arabic" },
	{ "E-L", "English Learning Programme" },
	{ "EO", "Esperanto" },
	{ "ES", "Estonian" },
	{ "Ewe", "Ewe" },
	{ "F", "French" },
	{ "FA", "Faroese" },
	{ "FI", "Finnish" },
	{ "FJ", "Fijian" },
	{ "FON", "Fon" },
	{ "FP", "Filipino" },
	{ "FS", "Farsi" },
	{ "FT", "Fiote" },
	{ "FU", "Fulani" },
	{ "FUJ", "FutaJalon" },
	{ "GA", "Garhwali" },
	{ "GD", "Greenlandic" },
	{ "GE", "Georgian" },
	{ "GJ", "Gujari" },
	{ "GL", "Galicic" },
	{ "GM", "Gamit" },
	{ "GO", "Gorontalo" },
	{ "GON", "Gondi" },
	{ "GR", "Greek" },
	{ "GU", "Gujarati" },
	{ "GUA", "Guaraní" },
	{ "GUR", "Gurage" },
	{ "HA", "Haussa" },
	{ "HAD", "Hadiya" },
	{ "HB", "Hebrew" },
	{ "HD", "Hindko" },
	{ "HI", "Hindi" },
	{ "HK", "Hakka" },
	{ "HM", "Hmong" },
	{ "HMA", "Hmar" },
	{ "HMB", "Hmong-Blue" },
	{ "HMW", "Hmong-White" },
	{ "HN", "Hani" },
	{ "HO", "Ho" },
	{ "HR", "Croatian" },
	{ "Hre", "Hre" },
	{ "HU", "Hungarian" },
	{ "HUA", "Huarani" },
	{ "HZ", "Hazaragi" },
	{ "I", "Italian" },
	{ "IB", "Iban" },
	{ "Ibo", "Ibo" },
	{ "IG", "Igbo" },
	{ "ILC", "Ilocano" },
	{ "ILG", "Ilonggo" },
	{ "IN", "Indonesian" },
	{ "INU", "Inuktikut" },
	{ "IRQ", "Iraqi" },
	{ "IS", "Icelandic" },
	{ "J", "Japanese" },
	{ "Jeh", "Jeh" },
	{ "JG", "Jingpho" },
	{ "JL", "Joula" },
	{ "JOR", "Jordanian" },
	{ "JR", "Jarai" },
	{ "JU", "Juba" },
	{ "JV", "Javanese" },
	{ "K", "Korean" },
	{ "KA", "Karen" },
	{ "K-P", "Karen-Pao" },
	{ "K-S", "Karen-Sgaw" },
	{ "K-W", "Karen-Pwo" },
	{ "KAD", "Kadazan" },
	{ "KAL", "Kalderash Romani" },
	{ "KAB", "Kabardino" },
	{ "KAM", "Kambaata" },
	{ "KAN", "Kannada" },
	{ "KAO", "Kaonde" },
	{ "KAR", "Karelian" },
	{ "KAT", "Katu" },
	{ "KAY", "Kayan" },
	{ "KB", "Kabyle" },
	{ "KBO", "Kok Borok" },
	{ "KC", "Kachin" },
	{ "KG", "Kyrgyz" },
	{ "KH", "Khmer" },
	{ "KHA", "Kham" },
	{ "KHM", "Khmu" },
	{ "KIM", "Kimwani" },
	{ "KiR", "KiRundi" },
	{ "KK", "KiKongo" },
	{ "KMB", "Kimbundu" },
	{ "KNK", "KinyaRwanda-KiRundi" },
	{ "KNU", "Kanuri" },
	{ "KO", "Kosovar Albanian" },
	{ "KOH", "Koho" },
	{ "KOM", "Komering" },
	{ "KON", "Konkani" },
	{ "KOY", "Koya" },
	{ "KPK", "Karakalpak" },
	{ "KRB", "Karbi" },
	{ "KRI", "Krio" },
	{ "KRW", "KinyaRwanda" },
	{ "KS", "Kashmiri" },
	{ "KU", "Kurdish" },
	{ "KuA", "Kurdish and Arabic" },
	{ "KuF", "Kurdish and Farsi" },
	{ "KUI", "Kui" },
	{ "KUL", "Kulina" },
	{ "K/k", "Kirmanji Kurdish" },
	{ "K/s", "Sorani Kurdish" },
	{ "KUK", "Kuki" },
	{ "KUM", "Kumaoni" },
	{ "KUN", "Kunama" },
	{ "KUR", "Kurukh" },
	{ "KUT", "Kutchi" },
	{ "KWA", "Kwanyama" },
	{ "KYK", "Kayan" },
	{ "KZ", "Kazakh" },
	{ "L", "Latin" },
	{ "LA", "Ladino" },
	{ "LAH", "Lahu" },
	{ "LAM", "Lampung" },
	{ "LAO", "Lao" },
	{ "LaS", "Ladino and Spanish" },
	{ "LB", "Lun Bawang" },
	{ "LBO", "Limboo" },
	{ "LEP", "Lepcha" },
	{ "LHO", "Lhotshampa" },
	{ "LIM", "Limba" },
	{ "LIN", "Lingala" },
	{ "LIS", "Lisu" },
	{ "LND", "Lunda Ndembo" },
	{ "LO", "Lomwe" },
	{ "LOZ", "Lozi" },
	{ "LT", "Lithuanian" },
	{ "LTO", "Oriental Liturgy of Vaticane Radio" },
	{ "LU", "Lunda" },
	{ "LUC", "Luchazi" },
	{ "LUN", "Lunyaneka" },
	{ "LUR", "Luri" },
	{ "LUV", "Luvale" },
	{ "LV", "Latvian" },
	{ "M", "Mandarin" },
	{ "MA", "Maltese" },
	{ "MAD", "Madurese" },
	{ "MAG", "Maghi" },
	{ "MAI", "Maithili" },
	{ "MAK", "Makonde" },
	{ "MAL", "Malayalam" },
	{ "MAM", "Mamay" },
	{ "MAN", "Manchu" },
	{ "MAO", "Maori" },
	{ "MAR", "Marathi" },
	{ "MAS", "Masaai" },
	{ "MC", "Macedonian" },
	{ "MCH", "Mavchi" },
	{ "MEI", "Meithei" },
	{ "MEN", "Mende" },
	{ "MEW", "Mewari" },
	{ "MIE", "Mien" },
	{ "MKB", "Minangkabau" },
	{ "MKS", "Makasar" },
	{ "MKU", "Makua" },
	{ "ML", "Malay" },
	{ "MLK", "Malinke" },
	{ "MNO", "Mnong" },
	{ "MO", "Mongolian" },
	{ "MOc", "Chinese" },
	{ "Mon", "Mon" },
	{ "MOO", "Moore" },
	{ "MOR", "Moro" },
	{ "MR", "Maronite" },
	{ "MRC", "Moroccan" },
	{ "MRI", "Mari" },
	{ "MRU", "Maru" },
	{ "MSY", "Malagasy" },
	{ "MUG", "Mugrabian" },
	{ "MUN", "Mundari" },
	{ "MUO", "Muong" },
	{ "MUR", "Murut" },
	{ "MW", "Marwari" },
	{ "MZ", "Mizo" },
	{ "NAG", "Naga-Makware" },
	{ "NDA", "Ndau" },
	{ "NDE", "Ndebele" },
	{ "NE", "Nepali" },
	{ "NG", "Nagpuri" },
	{ "NGA", "Ngangela" },
	{ "NI", "Ni" },
	{ "NIC", "Nicobari" },
	{ "NIU", "Niuean" },
	{ "NL", "Dutch" },
	{ "NO", "Norwegian" },
	{ "NP", "Nupe" },
	{ "NU", "Nuer" },
	{ "NUN", "Nung" },
	{ "NW", "Newari" },
	{ "NY", "Nyanja" },
	{ "OG", "Ogan" },
	{ "OH", "Otjiherero service in Namibia" },
	{ "OO", "Oromo" },
	{ "OR", "Oriya" },
	{ "OS", "Ossetic" },
	{ "OW", "Oshiwambo service in Angola and Namibia" },
	{ "P", "Portuguese" },
	{ "PAL", "Palaung - Pale" },
	{ "PED", "Pedi" },
	{ "PF", "Pashto and Farsi" },
	{ "PJ", "Punjabi" },
	{ "PO", "Polish" },
	{ "POT", "Pothwari" },
	{ "PS", "Pashto" },
	{ "PU", "Pulaar" },
	{ "Q", "Quechua" },
	{ "QQ", "Qashqai" },
	{ "R", "Russian" },
	{ "RAD", "Rade" },
	{ "REN", "Rengao" },
	{ "RH", "Rahanwein" },
	{ "RO", "Romanian" },
	{ "ROG", "Roglai" },
	{ "ROM", "Romanes" },
	{ "Ros", "Rosary session of Vaticane Radio" },
	{ "RWG", "Rawang" },
	{ "S", "Spanish" },
	{ "SAD", "Sadari" },
	{ "SAH", "Saho" },
	{ "SAM", "Samayiki" },
	{ "SAN", "Sango" },
	{ "SAS", "Sasak" },
	{ "SC", "Serbo-Croat" },
	{ "SCA", "Scandinavian languages" },
	{ "SD", "Sindhi" },
	{ "SED", "Sedang" },
	{ "SEF", "Sefardi" },
	{ "SEN", "Sena" },
	{ "SGA", "Shangaan" },
	{ "SGK", "Sgaw Karan" },
	{ "SGM", "Sara Gambai" },
	{ "SGO", "Songo" },
	{ "SHA", "Shan" },
	{ "SHk", "Shan-Khamti" },
	{ "SHC", "Sharchogpa" },
	{ "SHE", "Sheena" },
	{ "SHO", "Shona" },
	{ "SHP", "Sherpa" },
	{ "SHU", "Shuwa" },
	{ "SI", "Sinhalese" },
	{ "SID", "Sidamo" },
	{ "SIK", "Sikkimese" },
	{ "SIR", "Siraiki" },
	{ "SK", "Slovak" },
	{ "SLM", "Solomon Islands Pidgin" },
	{ "SM", "Samoan" },
	{ "SNK", "Sanskrit" },
	{ "SNT", "Santhali" },
	{ "SO", "Somali" },
	{ "SON", "Songhai" },
	{ "SOT", "SeSotho" },
	{ "SOU", "Sous" },
	{ "SR", "Serbian" },
	{ "SRA", "Soura" },
	{ "STI", "Stieng" },
	{ "SUD", "Sudanese" },
	{ "SUN", "Sundanese" },
	{ "SUS", "Sousou" },
	{ "SV", "Slovenian" },
	{ "SWA", "Swahili" },
	{ "SWE", "Swedish" },
	{ "SWT", "Swatow" },
	{ "SWZ", "SiSwati" },
	{ "SYL", "Syrian-Lebanese Arabic" },
	{ "T", "Thai" },
	{ "TAG", "Tagalog" },
	{ "TAH", "Tachelhit" },
	{ "TAM", "Tamil" },
	{ "TB", "Tibetan" },
	{ "TEL", "Telugu" },
	{ "TEM", "Temme" },
	{ "TFT", "Tarifit" },
	{ "TGR", "Tigre" },
	{ "THA", "Tharu Buksa" },
	{ "TIG", "Tigrinya" },
	{ "TJ", "Tajik" },
	{ "TK", "Turkmen" },
	{ "TL", "Tai-Lu" },
	{ "TM", "Tamazight" },
	{ "TMJ", "Tamajeq" },
	{ "TN", "Tai-Nua" },
	{ "TNG", "Tonga" },
	{ "TO", "Tongan" },
	{ "TOK", "Tokelau" },
	{ "TOR", "Torajanese" },
	{ "TP", "Tok Pisin" },
	{ "TS", "Tswana" },
	{ "TSA", "Tsangla" },
	{ "TSH", "Tshwa" },
	{ "TT", "Tatar" },
	{ "TTB", "Tatar-Bashkir" },
	{ "TU", "Turkish" },
	{ "TUL", "Tulu" },
	{ "TUM", "Tumbuka" },
	{ "TUN", "Tunisian" },
	{ "TUR", "Turki" },
	{ "TV", "Tuva" },
	{ "TW", "Taiwanese" },
	{ "Twi", "Twi" },
	{ "TZ", "Tamazight" },
	{ "UD", "Udmurt" },
	{ "UI", "Uighur" },
	{ "UK", "Ukrainian" },
	{ "UM", "Umbundu" },
	{ "UR", "Urdu" },
	{ "UZ", "Uzbek" },
	{ "V", "Vasco" },
	{ "Ves", "Vespers" },
	{ "Vn", "Vernacular = local languages" },
	{ "VN", "Vietnamese" },
	{ "VV", "Vasavi" },
	{ "VX", "Vlax Romani" },
	{ "W", "Wolof" },
	{ "Wa", "Wa" },
	{ "WE", "Wenzhou" },
	{ "WT", "White Tai" },
	{ "WU", "Wu" },
	{ "XH", "Xhosa" },
	{ "YAO", "Yao" },
	{ "Yi", "Yi" },
	{ "YI", "Yiddish" },
	{ "YK", "Yakutian" },
	{ "YO", "Yoruba" },
	{ "YOL", "Yolngu" },
	{ "YZ", "Yezidish" },
	{ "Z", "Zulu" },
	{ "ZA", "Zarma" },
	{ "ZD", "Zande" },
	{ "ZG", "Zaghawa" },
	{ "ZH", "Zhuang" },
	{ 0, 0}
	};
static struct { char *code; char *country; } countries[] = {
	{ "ABW", "Aruba" },
	{ "ADM", "Andaman & Nicobar Island" },
	{ "AFG", "Afghanistan" },
	{ "AFS", "South Africa" },
	{ "AGL", "Angola" },
	{ "AIA", "Anguilla" },
	{ "ALB", "Albania" },
	{ "ALG", "Algeria" },
	{ "ALS", "Alaska" },
	{ "AMS", "Saint Paul & Amsterdam Is." },
	{ "AND", "Andorra" },
	{ "ANO", "Annobon I." },
	{ "ARG", "Argentina" },
	{ "ARM", "Armenia" },
	{ "ARS", "Saudi Arabia" },
	{ "ASC", "Ascension I." },
	{ "ATA", "Antarctica" },
	{ "ATG", "Antigua" },
	{ "ATN", "Netherlands Leeward Antilles" },
	{ "ATW", "Netherlands Windward Antilles" },
	{ "AUS", "Australia" },
	{ "AUT", "Austria" },
	{ "AVE", "Aves I." },
	{ "AZE", "Azerbaijan" },
	{ "AZR", "Azores" },
	{ "B", "Brasil" },
	{ "BAH", "Bahamas" },
	{ "BAL", "Balleny Is." },
	{ "BAN", "Banaba" },
	{ "BDI", "Burundi" },
	{ "BEL", "Belgium" },
	{ "BEN", "Benin" },
	{ "BER", "Bermuda" },
	{ "BFA", "Burkina Faso" },
	{ "BGD", "Bangla Desh" },
	{ "BHR", "Bahrain" },
	{ "BIH", "Bosnia-Hertsegovina" },
	{ "BIO", "Chagos Is." },
	{ "BLR", "Belarus" },
	{ "BLZ", "Belize" },
	{ "BOL", "Bolivia" },
	{ "BOT", "Botswana" },
	{ "BOV", "Bouvet I." },
	{ "BRB", "Barbados" },
	{ "BRM", "Burma" },
	{ "BRU", "Brunei" },
	{ "BTN", "Bhutan" },
	{ "BUL", "Bulgaria" },
	{ "CAB", "Cabinda" },
	{ "CAF", "Central African Republic" },
	{ "CAN", "Canada" },
	{ "CBG", "Cambodia" },
	{ "CEU", "Ceuta" },
	{ "CGO", "Republic of Congo" },
	{ "CHL", "Chile" },
	{ "CHN", "China" },
	{ "CHR", "Chrstmas I." },
	{ "CKH", "Cook Is." },
	{ "CKN", "North Cook Is." },
	{ "CLM", "Colombia" },
	{ "CLN", "Sri Lanka" },
	{ "CLP", "Clipperton" },
	{ "CME", "Cameroon" },
	{ "CNR", "Canary Is." },
	{ "COM", "Comores" },
	{ "CPV", "Cape Verde Is." },
	{ "CRO", "Crozet I." },
	{ "CTI", "Ivory Coast" },
	{ "CTR", "Costa Rica" },
	{ "CUB", "Cuba" },
	{ "CVA", "Vatican State" },
	{ "CYM", "Cayman Is." },
	{ "CYP", "Gyprus" },
	{ "CZE", "Czech Republic" },
	{ "D", "Germany" },
	{ "DAI", "Daito Is." },
	{ "DES", "Desventurados Is." },
	{ "DJI", "Djibouti" },
	{ "DMA", "Dominica" },
	{ "DNK", "Denmarlc" },
	{ "DOM", "Dominican Republic" },
	{ "DRC",  "Democratic Republic of Congo" },
	{ "E", "Spain" },
	{ "EGY", "Egypt" },
	{ "EQA", "Ecuador" },
	{ "ERI", "Eritrea" },
	{ "EST", "Estonia" },
	{ "ETH", "Ethiopia" },
	{ "EUR", "Iles Europe & Bassas da India" },
	{ "F", "France" },
	{ "FAR", "Faroe Is." },
	{ "FIN", "Finland" },
	{ "FJI", "Fiji Is." },
	{ "FLK", "Falkland Is." },
	{ "FSM", "Federated States of Micronesia" },
	{ "G", "Great Britain and Northern Ireland" },
	{ "GAB", "Gabon" },
	{ "GDL", "Guadeloupe" },
	{ "GEO", "Georgia" },
	{ "GHA", "Ghana" },
	{ "GIB", "Gibraltar" },
	{ "GMB", "Gambia" },
	{ "GNB", "Guinea-Bissau" },
	{ "GNE", "Equatorial Guinea" },
	{ "GPG", "Galapagos Is." },
	{ "GRC", "Greece" },
	{ "GRD", "Grenada" },
	{ "GRL", "Greenland" },
	{ "GTB", "Guantanamo Bay" },
	{ "GTM", "Guatemala" },
	{ "GUF", "French Guyana" },
	{ "GUI", "Guinea" },
	{ "GUM", "Guam" },
	{ "GUY", "Guyana" },
	{ "HKG", "Hong Kong, part of China" },
	{ "HMD", "Heard & McDonaid Is." },
	{ "HND", "Honduras" },
	{ "HNG", "Hungary" },
	{ "HOL", "Netherlands" },
	{ "HRV", "Croatia" },
	{ "HTI", "Haiti" },
	{ "HWA", "Hawaii Is." },
	{ "HWL", "Howland & Baker Is." },
	{ "I", "Italy" },
	{ "ICO", "Cocos I." },
	{ "IND", "India" },
	{ "INS", "Indonesia" },
	{ "IRL", "Ireland" },
	{ "IRN", "Iran" },
	{ "IRQ", "Iraq" },
	{ "ISL", "Iceland" },
	{ "ISR", "Israel" },
	{ "IWA", "Ogasawara" },
	{ "J", "Japan" },
	{ "JAF", "Jarvis" },
	{ "JDN", "Juan de Nova Island" },
	{ "JMC", "Jamaica" },
	{ "JMY", "Jan Mayen" },
	{ "JON", "Johnston Atoll" },
	{ "JOR", "Jordan" },
	{ "JUF", "Juan Femandez I." },
	{ "KAL", "Kaliningrad" },
	{ "KAZ", "Kazakstan" },
	{ "KEN", "Kenya" },
	{ "KER", "Kerguelen" },
	{ "KGZ", "Kyrgyzstan" },
	{ "KIR", "Kiribati" },
	{ "KOR", "Korea, South" },
	{ "KRE", "Korea, North" },
	{ "KWT", "Kuwait" },
	{ "LAO", "Laos" },
	{ "LBN", "Lebanon" },
	{ "LBR", "Liberia" },
	{ "LBY", "Libya" },
	{ "LCA", "Saint Lucia" },
	{ "LIE", "Liechtenstein" },
	{ "LSO", "Lesotho" },
	{ "LTU", "Lithuania" },
	{ "LUX", "Luxembourg" },
	{ "LVA", "Latvia" },
	{ "MAC", "Macao" },
	{ "MAU", "Mauritius" },
	{ "MCO", "Monaco" },
	{ "MDA", "Moldova" },
	{ "MDG", "Madagascar" },
	{ "MDR", "Madeira" },
	{ "MEL", "Melilla" },
	{ "MEX", "Mexico" },
	{ "MKD", "Macedonia" },
	{ "MLA", "Malaysia" },
	{ "MLD", "Maldives" },
	{ "MLI", "Mali" },
	{ "MLT", "Malta" },
	{ "MNE", "Montenegro" },
	{ "MNG", "Mongolia" },
	{ "MOZ", "Mozambique" },
	{ "MRA", "Northern Mariana Islands" },
	{ "MRC", "Morocco" },
	{ "MRL", "Marshail Is." },
	{ "MRN", "Marion & Prince Edward Is." },
	{ "MRQ", "Marquesas Is." },
	{ "MRT", "Martinique" },
	{ "MSR", "Montserrat" },
	{ "MTN", "Mauritania" },
	{ "MWI", "Malawi" },
	{ "MYT", "Mayotte" },
	{ "NCG", "Nicaragua" },
	{ "NCL", "New Caledonia" },
	{ "NFK", "Norfolk I." },
	{ "NGR", "Niger" },
	{ "NIG", "Nigeria" },
	{ "NIU", "Niue" },
	{ "NMB", "Namibia" },
	{ "NOR", "Norway" },
	{ "NPL", "Nepal" },
	{ "NRU", "Nauru" },
	{ "NZL", "New Zealand" },
	{ "OCE", "Society Is." },
	{ "OMA", "Oman" },
	{ "PAK", "Pakistan" },
	{ "PAL", "Palau" },
	{ "PAQ", "Easter Island" },
	{ "PHL", "Philippines" },
	{ "PHX", "Phoenix Is." },
	{ "PLM", "Palmyra" },
	{ "PNG", "Papua New Guinea" },
	{ "PNR", "Panama Republic" },
	{ "POL", "Poland" },
	{ "POR", "Portugal" },
	{ "PRG", "Paraguay" },
	{ "PRU", "Peru" },
	{ "PRV", "Okino-Tori-Shima" },
	{ "PTC", "Pitcairn" },
	{ "PTR", "Puerto Rico" },
	{ "QAT", "Qatar" },
	{ "REU", "Reunion" },
	{ "ROU", "Roumania" },
	{ "RRW", "Rwanda" },
	{ "RUS", "Russian Federation" },
	{ "S", "Sweden" },
	{ "SAP", "San Andres & Providencia" },
	{ "SCN", "Saint Kitts - Nevis" },
	{ "SCO", "Scott I." },
	{ "SDN", "Sudan" },
	{ "SEN", "Senegal" },
	{ "SEY", "Seychelles" },
	{ "SGA", "South Georgia Is." },
	{ "SHN", "Saint Helena I." },
	{ "SLM", "Solomon Is." },
	{ "SLV", "EI Salvador" },
	{ "SMA", "Samoa" },
	{ "SMO", "Samoa" },
	{ "SMR", "San Marino" },
	{ "SNG", "Singapore" },
	{ "SOK", "South Orkney Is." },
	{ "SOM", "Somalia" },
	{ "SPM", "Saint Pierre & Miquelon" },
	{ "SPO", "Sao Paulo I." },
	{ "SPR", "Spratly Is." },
	{ "SRB", "Serbia" },
	{ "SRL", "Sierra Leone" },
	{ "SSI", "South Sandwich Is." },
	{ "STB", "Saint Barthelemy" },
	{ "STP", "Sao Tome & Principe" },
	{ "SUI", "Switzerland" },
	{ "SUR", "Suriname" },
	{ "SVB", "Svalbard" },
	{ "SVK", "Slovakia" },
	{ "SVN", "Slovenia" },
	{ "SWZ", "Swaziland" },
	{ "SYR", "Syria" },
	{ "TCD", "Tchad" },
	{ "TCI", "Turks & Caicos Is." },
	{ "TGO", "Togo" },
	{ "THA", "Thaiiand" },
	{ "TJK", "Tadjikistan" },
	{ "TKL", "Tokelau I." },
	{ "TKM", "Turkmenistan" },
	{ "TON", "Tonga" },
	{ "TRC", "Tristan da Cunha" },
	{ "TRD", "Trinidad & Tobago" },
	{ "TRI", "Trindade & Martim Vaz Is.," },
	{ "TUA", "Tuamotu Archipelago" },
	{ "TUN", "Tunisia" },
	{ "TUR", "Turkey" },
	{ "TUV", "Tuvalu" },
	{ "TWN", "Taiwan" },
	{ "TZA", "Tanzania" },
	{ "UAE", "United Arab Emirates" },
	{ "UGA", "Uganda" },
	{ "UKR", "Ukraine" },
	{ "URG", "Uruguay" },
	{ "USA", "United States of America" },
	{ "UZB", "Uzbekistan" },
	{ "VCT", "Saint Vincent" },
	{ "VEN", "Venezuela" },
	{ "VIR", "American Virgin Is." },
	{ "VRG", "British Virgin Is." },
	{ "VTN", "Vietnam" },
	{ "VUT", "Vanuatu" },
	{ "WAK", "Wake I." },
	{ "WAL", "Waliis & Futuna Is." },
	{ "YEM", "Yemen" },
	{ "ZMB", "Zambia" },
	{ "ZWE", "Zimbabwe" },
	{ 0, 0}
	};
static struct { char *code; char *target; } targets[] = {
	{ "Af", "Africa" },
	{ "Am", "America" },
	{ "As", "Asia" },
	{ "Car", "Caribbean" },
	{ "Cau", "Caucasus" },
	{ "CIS", "Commonwealth of Independent States" },
	{ "CNA", "Central North America" },
	{ "ENA", "Eastern North America" },
	{ "Eu", "Europe" },
	{ "FE", "Far East" },
	{ "LAm", "Latin America" },
	{ "ME", "Middle East" },
	{ "NAO", "North Atlantic Ocean" },
	{ "Oc", "Oceania" },
	{ "SAO", "South Atlantic Ocean" },
	{ "SEA", "South East Asia" },
	{ "SEE", "South East Europe" },
	{ "Sib", "Siberia" },
	{ "WNA", "Western North America" },
	{ 0, 0}
	};

	/*
	 *  One-letter or two-letter codes for different transmitter sites within one country.
	 *  No such code is used if there is only one transmitter site in that country.
	 *          
	 *  The code is used plainly if the transmitter is located within the home country of the station.
	 *  Otherwise, it is preceeded by "/ABC"
	 *  where ABC is the ITU code of the host country of the transmitter site.
	 *  Example:
	 *    A BBC broadcast, relayed by the transmitters in Samara, Russia, would be designated as "/RUS-s".
	 *  In many cases, a station has a "major" transmitter site which is normally not designated.
	 *  Should this station use a different transmitter site in certain cases, they are marked.
	 *  No transmitter-site code is used when the transmitter site is not known.
	 */
static struct { char *country; char *mark; char * bc; char *site;} stations[] = {
	{ "AFS", "", "", "Meyerton 26S35-28E08" },
	{ "AGL", "", "", "Mulenvos 08S53-13E20" },
	{ "AIA", "", "", "The Valley 18N13-63W01" },
	{ "ALB", "", "CRI", "Cerrik 40N59'47-19E59'58 (1x100kW = 2x50kW)" },
	{ "ALB", "", "R Tirana", "Shijiak 41N19'535-19E33'086 (1x100kW = 2x50kW)" },
	{ "ALB", "", "MW", "Durres/Fllake 41N22'11-19E30'17 (500 kW)" },
	{ "ALS", "", "", "Anchor Point 59N45-151W44" },
	{ "ARG", "", "", "General Pacheco 34S36-58W22" },
	{ "ARM", "", "", "Gavar (formerly Kamo) 40N25-45E11" },
	{ "ARS", "", "", "Riyadh 24N30-46E23" },
	{ "ARS", "j", "", "Jeddah 21N32-39E10" },
	{ "ASC", "", "", "Ascension Island, 07S54-14W23" },
	{ "ATA", "", "", "Base Esperanza 63S24-56W59" },
	{ "ATG", "", "", "St.John's 17N06-61W48 (2 x 250kW)" },
	{ "ATN", "", "", "Bonaire 12N12-68W18" },
	{ "AUS", "", "", "Shepparton 36S20-145E25" },
	{ "AUS", "a", "", "Alice Springs 23S49-133E51" },
	{ "AUS", "b", "", "Brandon 19S31-147E20" },
	{ "AUS", "d", "", "Darwin NT 12S25-136E37" },
	{ "AUS", "h", "", "Humpty Doo NT 12S34-131E05" },
	{ "AUS", "ka", "", "Katherine NT 14S24-132E11" },
	{ "AUS", "ku", "", "Kununurra WA 15S48-128E41" },
	{ "AUS", "t", "", "Tennant Creek NT 19S40-134E16" },
	{ "AUT", "", "", "Moosbrunn 48N00-16E28" },
	{ "AZE", "", "", "Gäncä 40N37-46E20" },
	{ "BEL", "", "", "Wavre(Waver) 50N44-04E34" },
	{ "BEN", "", "", "Parakou 09N20-02E38" },
	{ "BGD", "", "", "Dhaka 23N43-90E26" },
	{ "BHR", "", "", "Abu Hayan 26N02-50E37" },
	{ "BIH", "", "", "Bijeljina 44N42-19E10" },
	{ "BIO", "", "", "Diego Garcia 07S03-72E04" },
	{ "BLR", "", "", "Minsk-Sasnovy 53N56-27E34" },
	{ "BLR", "b", "", "Brest 52N20-23E35 (5kW)" },
	{ "BLR", "g", "", "Hrodna/Grodno 53N40-23E50(5 kW)" },
	{ "BLR", "m", "", "Mahiliou/Mogilev (Orsha) 53N37-30E20 (5 kW)" },
	{ "BOT", "", "VoA", "Mopeng Hill 21S57-27E39" },
	{ "BTN", "", "", "Thimphu 27N28-89E39" },
	{ "BUL", "", "", "Plovdiv-Padarsko 42N10-24E42  (2x500kW, 3x250kW)" },
	{ "BUL", "s", "", "Sofia-Kostinbrod 42N49-23E13  (2x100kW, 2x50kW)" },
	{ "BUL", "v", "", "Varna 43N03-27E40" },
	{ "BUL", "#", "747", "Petrich 41N42-23E18  (500kW)" },
	{ "BUL", "#", "1224", "Vidin 43N49-22E40  (500kW)" },
	{ "CAN", "", "", "Sackville 45N53-64W19" },
	{ "CAN", "c", "", "Calgary AB" },
	{ "CAN", "j", "", "St John's NL 47N34-52W48" },
	{ "CAN", "o", "", "Ottawa ON 40N18-75W45" },
	{ "CAN", "t", "", "Toronto (Aurora) ON 44N00-79W30" },
	{ "CAN", "v", "", "Vancouver BC 49N11-123W04" },
	{ "CBG", "", "", "Phnom Penh 11N34-104E51" },
	{ "CHL", "", "", "Santiago 33S27-70W41" },
	{ "CHN", "a", "", "Baoji 34N30-107E10 (5x100kW)" },
	{ "CHN", "b", "", "Beijing 39N57-116E27" },
	{ "CHN", "B", "", "Beijing 39N55-116E25 (12x100kW, 9x50kW)" },
	{ "CHN", "d", "", "Dongfang (Hainan) 18N54-108E39 (150kW)" },
	{ "CHN", "e", "", "Gejiu (Yunnan) 23N21-103E08" },
	{ "CHN", "g", "", "Gannan (Hezuo) 35N06-102E54" },
	{ "CHN", "h", "", "Hohhot (Nei Menggu) 41N12-111E30" },
	{ "CHN", "j", "", "Jinhua 28N07-119E39" },
	{ "CHN", "k", "", "Kunming (Yunnan) 25N10-102E50" },
	{ "CHN", "ka", "", "Kashi (Kashgar) (Xinjiang) 39N30-76E00" },
	{ "CHN", "L", "", "Lingshi (Shanxi) 36N52-111E40 (6x100kW)" },
	{ "CHN", "n", "", "Nanning (Guangxi) 22N47-108E11" },
	{ "CHN", "q", "", "Ge'ermu (Qinghai) 36N24-94E59 (100kW)" },
	{ "CHN", "qq", "", "Qiqihar 47N02-124E03 (500kW)" },
	{ "CHN", "s", "", "Shijiazhuang (Hebei) 38N04-114E28 (5x50kW)" },
	{ "CHN", "t", "", "Tibet (Lhasa) 29N30-90E59" },
	{ "CHN", "u", "", "Urumqi (Xinjiang) 43N35-87E30 (6x500kW)" },
	{ "CHN", "x", "", "Xian (Shaanxi) 34N12-108E54" },
	{ "CHN", "#", "603", "Guangdong" },
	{ "CHN", "#", "684", "Dongfang" },
	{ "CHN", "#", "1296", "Kunming" },
	{ "CHN", "#", "", "Fuzhou:26N06-119E24" },
	{ "CHN", "#", "", "Lhasa:29N30-90E59" },
	{ "CHN", "#", "", "Nanchang:28N38-115E56" },
	{ "CHN", "#", "", "Nanjing:32N02-118E44" },
	{ "CHN", "#", "", "Nanning:32N02-108E11" },
	{ "CHN", "#", "", "Qinghai: Xining 36N38-101E36" },
	{ "CLN", "", "DW", "Trincomalee 08N44-81E10 (SW 3 x 250kW, MW 400kW)" },
	{ "CLN", "#", "", "R.Japan/SLBC" },
	{ "CLN", "#", "", "RL/VoA" },
	{ "CTR", "", "Gene Scott", "Cahuita 09N45-82W54" },
	{ "REE", "", "", "Caiari de Porocí 10N00-83W30" },
	{ "CUB", "", "", "La Habana (Quivicán) 22N50-82W17" },
	{ "CVA", "", "", "Santa Maria di Galeria 42N03-12E19" },
	{ "CVA", "v", "", "Citta del Vaticano 41N54-12E27" },
	{ "CYP", "", "", "Zygi (Limassol) 34N43-33E19" },
	{ "CYP", "y", "", "Yeni Iskele 35N13-33E55" },
	{ "CZE", "", "", "Litomysl 49N48-16E10" },
	{ "D", "", "", "unknown German station TBD" },
	{ "D", "b", "", "Biblis 49N41-08E29" },
	{ "D", "be", "", "Berlin 52N30-13E20" },
	{ "D", "d", "", "Dillberg 49N19-11E23" },
	{ "D", "e", "", "Erlangen 49N35-11E00" },
	{ "D", "h", "", "Holzkirchen 47N52-11E44" },
	{ "D", "ha", "", "Hannover 52N23-09E42" },
	{ "D", "i", "", "Ismaning 48N15-11E45" },
	{ "D", "j", "", "Juelich 50N57-06E22 (100kW)" },
	{ "D", "L", "", "Lampertheim 49N36-08E33" },
	{ "D", "n", "", "Nauen 52N38-12E54 ( 4 x 500kW)" },
	{ "D", "nu", "", "Nuernberg 49N27-11E05" },
	{ "D", "w", "", "Wertachtal 48N05-10E41  (13 x 500kW)" },
	{ "DJI", "", "", "Djibouti 11N30-43E00" },
	{ "DNK", "", "", "Karup 56N18-09E10" },
	{ "E", "", "", "Noblejas 39N57-03W26" },
	{ "EGY", "", "", "unknown Egypt" },
	{ "EGY", "a", "", "Abis 31N10-30E05" },
	{ "EGY", "z", "", "Abu Zaabal 30N16-31E22" },
	{ "ETH", "", "R.Ethiopia", "Geja Jewe 08N47-38E38" },
	{ "F", "", "", "Issoudun 46N57-01E59" },
	{ "F", "r", "", "Rennes 48N06-01W41" },
	{ "FIN", "", "YLE", "Pori 61N28-21E35" },
	{ "FIN", "#", "Scand.Weekend R.", "" },
	{ "G", "", "", "unknown UK" },
	{ "G", "r", "", "Rampisham 50N48-02W38" },
	{ "G", "s", "", "Skelton 54N44-02W54" },
	{ "G", "w", "", "Woofferton 52N19-02W43" },
	{ "G", "cp", "", "London-Crystal Palace" },
	{ "G", "cr", "", "London-Croydon" },
	{ "G", "", "648", "Orfordness" },
	{ "G", "", "1296", "Orfordness" },
	{ "G", "", "198", "Droitwich" },
	{ "GAB", "", "", "Moyabi 01S40-13E31" },
	{ "GEO", "", "", "Dusheti 42N03-44E41" },
	{ "GNE", "", "", "Bata 01N48-09E46" },
	{ "GRC", "", "", "Avlis (38N23-23E36)" },
	{ "GUF", "", "", "Montsinery 04N54-52W36" },
	{ "GUM", "", "AWR", "Agat, 13N20-144E39" },
	{ "KTWR", "", "", "Agana 13N17-144E40" },
	{ "AFRTS", "", "", "Barrigada 13N34-144E50" },
	{ "GUY", "", "", "Sparendaam 06N49-58W10" },
	{ "HNG", "", "", "Jaszbereny 47N35-19E52" },
	{ "HOL", "", "", "Flevo 52N21-05E27" },
	{ "HRV", "", "", "Deanovec 45N41-16E27" },
	{ "HWA", "", "", "Naalehu 19N02-155W40" },
	{ "AFRTS", "", "", "Pearl Harbour 21N25-158W09" },
	{ "I", "", "RAI, VoM", "Roma (Prato Smeraldo) 41N48-12E31" },
	{ "IRRS", "", "", "unknown. Formerly Milano 45N27-09E11" },
	{ "IND", "", "", "unknown India" },
	{ "IND", "a", "", "Aligarh 28N00-78E06" },
	{ "IND", "b", "", "Bengaluru-Doddaballapur (Bangalore) 13N14-77E13" },
	{ "IND", "c", "", "Chennai(ex Madras) 13N08-80E07" },
	{ "IND", "d", "", "Delhi (Kingsway) 26N45-77E12" },
	{ "IND", "g", "", "Gorakhpur 23N52-83E28" },
	{ "IND", "k", "", "Delhi(Khampur) 28N43-77E38" },
	{ "IND", "m", "", "Mumbai (ex Bombay) 19N11-72E49" },
	{ "IND", "p", "", "Panaji (ex Goa) 15N28-73E51" },
	{ "IND", "w", "", "Guwahati 26N11-91E50" },
	{ "IND", "#", "594", "Kolkata(Calcutta)-Chinsurah" },
	{ "IND", "#", "1134", "Kolkata(Calcutta)-Chinsurah" },
	{ "IND", "#", "", "702-Jalandhar" },
	{ "IND", "#", "", "1053-Tuticorin" },
	{ "IND", "#", "", "1071-Rajkot" },
	{ "IND", "#", "", "Regional Stations" },
	{ "IND", "#", "", "Aizawl(10kW)" },
	{ "IND", "#", "", "Bhopal(50kW)" },
	{ "IND", "#", "", "Chennai(50kW)" },
	{ "IND", "#", "", "Delhi(Kingsway)(50kW)" },
	{ "IND", "#", "", "Gangtok(10kW)" },
	{ "IND", "#", "", "Guwahati(50kW)" },
	{ "IND", "#", "", "Hyderabad(50kW)" },
	{ "IND", "#", "", "Imphal(50kW)" },
	{ "IND", "#", "", "Itanagar(50kW)" },
	{ "IND", "#", "", "Jaipur(50kW)" },
	{ "IND", "#", "", "Jammu(50kW)" },
	{ "IND", "#", "", "Jeypore(50kW)" },
	{ "IND", "#", "", "Kohima(50kW)" },
	{ "IND", "#", "", "Kolkata(Calcutta)(50kW)" },
	{ "IND", "#", "", "Kurseong(50kW)" },
	{ "IND", "#", "", "Leh(10kW)" },
	{ "IND", "#", "", "Lucknow(50kW)" },
	{ "IND", "#", "", "Mumbai(50kW)" },
	{ "IND", "#", "", "Port Blair-Brookshabad(5kW)" },
	{ "IND", "#", "", "Ranchi(50kW)" },
	{ "IND", "#", "", "Shillong(50kW)" },
	{ "IND", "#", "", "Shimla(50kW)" },
	{ "IND", "#", "", "Srinagar(50kW)" },
	{ "IND", "#", "", "Thiruvananthapuram(Trivendrum)(50kW)" },
	{ "INS", "", "", "Jakarta (Cimanggis) 06S12-106E51" },
	{ "IRN", "", "", "unknown Iran" },
	{ "IRN", "a", "", "Ahwaz 31N20-48E40" },
	{ "IRN", "k", "", "Kamalabad 35N46-51E27" },
	{ "IRN", "m", "", "Mashhad 36N15-59E33" },
	{ "IRN", "s", "", "Sirjan 29N27-55E41" },
	{ "IRN", "z", "", "Zahedan 29N28-60E53" },
	{ "ISL", "", "", "Reykjavík 64N05-21W50" },
	{ "ISR", "", "", "Yavne 31N52-34E45" },
	{ "J", "", "", "Yamata 36N10-139E50" },
	{ "JOR", "", "", "Al Karanah 31N44-36E26" },
	{ "KAZ", "", "", "Almaty (Alma Ata, Dmitrievka) 43N17-77E00" },
	{ "KAZ", "k", "", "Karaturuk 43N39-77E56" },
	{ "KGZ", "", "", "Bishkek 42N54-74E37" },
	{ "KOR", "", "", "Kimjae 35N50-126E50" },
	{ "KOR", "h", "", "Hwasung 37N13-126E47" },
	{ "KRE", "k", "", "Kanggye 40N58-126E36" },
	{ "KRE", "p", "", "Pyongyang 39N05-125E23" },
	{ "KRE", "u", "", "Kujang 40N05-125E05" },
	{ "KWT", "", "", "Sulaibiyah 29N10-47E45" },
	{ "LAO", "", "", "Vientiane 17N58-102E33" },
	{ "LTU", "", "", "Sitkunai 55N02-23E49 http" },
	{ "LUX", "", "", "Junglinster 49N43-06E15" },
	{ "LVA", "", "", "Ulbroka 56N56-24E17" },
	{ "MCO", "", "", "Mont Angel/Fontbonne 43N44-07E26" },
	{ "MDA", "", "", "Maiac near Grigoriopol 47N14-29E24" },
	{ "MDG", "", "", "Talata Volonondry 18S43-47E37" },
	{ "MLA", "", "", "unknown MLA" },
	{ "MLA", "ka", "", "Kajang 03N01-101E46" },
	{ "MLA", "kk", "", "Kota Kinabalu 06N12-116E14" },
	{ "MLA", "ku", "", "Kuching 01N33-110E20" },
	{ "MLA", "s", "", "Sibu 02N18-111E49" },
	{ "MLI", "", "", "Bamako 12N39-08W01" },
	{ "MNG", "", "", "Ulaanbaatar 47N55-107E00" },
	{ "MRA", "", "RFA/RL/VoA", "Tinian 15N03-145E36" },
	{ "MRA", "s", "", "Saipan (Agingan Point) 15N07-145E42" },
	{ "KFBS", "", "Marpi", "Saipan 15N16-145E48" },
	{ "MRC", "", "VoA/RL/RFE", "Briech 35N34-05W58" },
	{ "MRC", "t", "RTM", "Tanger 35N48-05W55" },
	{ "MRC", "n", "Medi-1", "Nador 35N03-02W55" },
	{ "NOR", "", "", "unknown Norway" },
	{ "NOR", "k", "", "Kvitsoy 59N04-05E27" },
	{ "NOR", "s", "", "Sveio 59N37-05E19" },
	{ "NZL", "", "", "Rangitaiki 38S50-176E25" },
	{ "OMA", "", "BBC", "A'Seela 21N57-59E27" },
	{ "OMA", "t", "Radio Oman", "Thumrait 17N38-53E56" },
	{ "OMA", "s", "", "Seeb 23N40-58E10" },
	{ "PAK", "", "", "Islamabad 33N27-73E12" },
	{ "PAK", "p", "", "Peshawar 34N00-71E30 (10kW)" },
	{ "PAK", "q", "", "Quetta 30N15-67E00 (10kW)" },
	{ "PAK", "r", "", "Rawalpindi 33N30-73E00 (10kW)" },
	{ "PAL", "", "", "Medorn 07N22-134E28" },
	{ "PHL", "", "RL/Voa", "Tinang 15N21-120E37" },
	{ "PHL", "x", "", "Tinang-2(portable) 15N21-120E38 (50kW)" },
	{ "PHL", "#", "1143", "Poro 16N26-120E17" },
	{ "FEBC", "", "", "Bocaue 14N48-120E55" },
	{ "FEBC", "i", "", "Iba 15N22-119E57" },
	{ "FEBC", "#", "RVA, R Vaticana", "Palauig, Zembales 15N28-119E50" },
	{ "FEBC", "#", "", "DUR2 9581 14N41-120E59" },
	{ "POL", "", "", "Warszawa 52N04-20E52" },
	{ "POR", "", "DW", "Sines 37N57-08W45 (3 x 250kW)" },
	{ "RdP", "", "", "Sao Gabriel(Lisboa) 38N45-08W40" },
	{ "PTR", "", "", "Roosevelt Roads 18N23-67W11" },
	{ "ROU", "", "", "Tiganesti 44N42-26E06" },
	{ "ROU", "t", "", "Tiganesti 44N42-26E06" },
	{ "ROU", "g", "", "Galbeni 46N44-26E50" },
	{ "ROU", "s", "", "Saftica 50kW" },
	{ "RUS", "", "", "unknown Russia" },
	{ "RUS", "a", "", "Armavir/Tblisskaya/Krasnodar 45N00-40E49" },
	{ "RUS", "ar", "", "Arkhangelsk 64N30-40E30" },
	{ "RUS", "b", "", "Blagoveshchensk (Amur) 50N16-127E30" },
	{ "RUS", "c", "", "Chita (Atamanovka) (S Siberia) 51N50-113E43" },
	{ "RUS", "e", "", "Ekaterinburg (S Ural) 56N55-60E36" },
	{ "RUS", "i", "", "Irkutsk (Angarsk) (S Siberia) 52N18-104E18" },
	{ "RUS", "k", "", "Kaliningrad-Bolshakovo 54N54-21E43" },
	{ "RUS", "ka", "", "Komsomolsk-na-Amur (Far East) 50N39-136E55" },
	{ "RUS", "kh", "", "Khabarovsk (Far East) 48N33-135E15" },
	{ "RUS", "kr", "", "Krasnoyarsk 56N01-92E54" },
	{ "RUS", "ku", "", "Kurovskaya-Avsyunino (near Moscow) 55N34-39E09" },
	{ "RUS", "m", "", "Moscow/Moskva 55N45-37E18" },
	{ "RUS", "ma", "", "Magadan" },
	{ "RUS", "mu", "", "Murmansk 68N58-32E46" },
	{ "RUS", "n", "", "Novosibirsk (Oyash) 55N31-83E45" },
	{ "RUS", "n", "DW", "Novosibirsk City 55N04-82E58" },
	{ "RUS", "p", "", "Petropavlovsk-Kamchatskij (Yelizovo) 52N59-158E39" },
	{ "RUS", "s", "", "Samara (Zhygulevsk) 53N17-50E15" },
	{ "RUS", "se", "", "Serpukhov [actually Noginsk but Noginsk closed with start of A03] 54N54-37E25" },
	{ "RUS", "sp", "", "St.Petersburg (Popovka/Krasnyj Bor) 59N39-30E42" },
	{ "RUS", "t", "", "Taldom - Severnyj (near Moscow) 56N44-37E38" },
	{ "RUS", "u", "", "Ulan-Ude" },
	{ "RUS", "v", "", "Vladivostok (Razdolnoye) 43N32-131E57" },
	{ "RUS", "ya", "", "Yakutsk/Tulagino 62N01-129E48" },
	{ "RUS", "ys", "", "Yuzhno-Sakhalinsk (Vestochka) 46N55-142E54" },
	{ "RRW", "", "", "Kigali 01S53-30E07 (4 x 250kW)" },
	{ "S", "", "", "Hoerby 55N49-13E44" },
	{ "SEY", "", "", "Mahe 04S36-55E28" },
	{ "SNG", "", "", "Kranji 01N25-103E44" },
	{ "SRB", "s", "", "Stubline 44N34-20E09" },
	{ "SRL", "", "SLBS", "Goderich 08N30-13W14" },
	{ "STP", "", "", "Pinheira 00N18-06E46" },
	{ "SVK", "", "", "Rimavska Sobota 48N23-20E00" },
	{ "SWZ", "", "", "Manzini 26S34-31E59" },
	{ "SYR", "", "", "Adra 33N27-36E30" },
	{ "TCD", "", "", "N'Djamena-Gredia 12N08-15E03" },
	{ "THA", "", "RL/R.THA/VoA", "Udon Thani 17N25-102E48" },
	{ "THA", "b", "", "Bangkok / Prathum Thani(1575, 4830, 6070, 7115) 13N47-100E30" },
	{ "THA", "", "BBC", "Nakhon Sawan 15N49-100E04" },
	{ "TJK", "", "", "Yangi Yul (Dushanbe) 38N29-68E48" },
	{ "TJK", "y", "", "Yangi Yul (Dushanbe) 38N29-68E48" },
	{ "TJK", "o", "", "Orzu 37N32-68E42" },
	{ "TUN", "", "", "Sfax 34N48-10E53" },
	{ "TUR", "", "", "Emirler 39N29-32E51" },
	{ "TUR", "c", "", "Cakirlar 39N58-32E40" },
	{ "TWN", "", "", "Huwei 23N43-120E25" },
	{ "TWN", "h", "", "Huwei 23N43-120E25" },
	{ "TWN", "k", "", "Kouhu 23N35-120E10" },
	{ "TWN", "m", "", "Minhsiung 23N29-120E27" },
	{ "TWN", "n", "", "Tainan 23N11-120E38" },
	{ "TWN", "p", "", "Paochung 23N43-120E18" },
	{ "TWN", "s", "", "Tanshui 25N13-121E29" },
	{ "TWN", "t", "", "Taipei (Pali) 25N05-121E27" },
	{ "UAE", "", "", "Dhabbiya 24N11-54E14" },
	{ "UAE", "m", "", "Makta 24N21-54E34" },
	{ "UAE", "", "UAE R Dubai", "Dubai 25N14-55E16" },
	{ "UKR", "", "", "Kiev/Brovary 50N31-30E46" },
	{ "UKR", "k", "", "Kiev/Brovary 50N31-30E46" },
	{ "UKR", "L", "", "Lviv (Krasne) (Russian name Lvov) 49N51-24E40" },
	{ "UKR", "m", "", "Mykolaiv (Kopani) (Russian name Nikolayev) 46N49-32E14" },
	{ "UKR", "x", "", "Kharkiv (Taranivka) (Russian name Kharkov) 49N38-36E07" },
	{ "UKR", "z", "", "Zaporizhzhya" },
	{ "UKR", "", "657", "Chernivtsi" },
	{ "UKR", "", "936", "Krasne" },
	{ "USA", "", "", "unknown USA" },
	{ "USA", "c", "", "Cypress Creek, SC 32N41-81W08" },
	{ "USA", "d", "", "Delano, CA 35N45-119W10" },
	{ "USA", "g", "", "Greenville, NC 35N35-77W22" },
	{ "USA", "k", "", "Key Saddlebunch, FL 24N34-81W45" },
	{ "USA", "o", "", "Okeechobee, FL 27N28-80W56" },
	{ "USA", "w", "", "via WWCR, Nashville" },
	{ "USA", "", "KAIJ", "Dallas, TX 33N13-96W52" },
	{ "USA", "", "KJES", "Vado, NM 32N08-106W35" },
	{ "USA", "", "KTBN", "Salt Lake City, UT 40N39-112W03" },
	{ "USA", "", "KVOH", "Los Angeles (Rancho Simi), CA 34N15-118W38" },
	{ "USA", "", "WBCQ", "Monticello, ME 46N20-67W50" },
	{ "USA", "", "WBOH", "Newport, NC 34N47-76W56" },
	{ "USA", "", "WEWN", "Birmingham (Vandiver), AL 33N30-86W28" },
	{ "USA", "", "WGTG", "McGaysville, GA 34N58-84W22" },
	{ "USA", "", "WHRA", "Greenbush, ME 45N08-68W34" },
	{ "USA", "", "WHRI", "Cypress Creek, SC 32N41-81W08" },
	{ "USA", "", "WINB", "Red Lion (York), PA 39N54-76W35" },
	{ "USA", "", "WJIE", "Millerstown, KY 37N26-86W02" },
	{ "USA", "", "WMLK", "Bethel, PA 40N29-76W17" },
	{ "USA", "", "WRMI", "Miami (Hialeah Gardens), FL 25N54-80W22" },
	{ "USA", "", "WRNO", "New Orleans, LA 29N50-90W07" },
	{ "USA", "", "WTJC", "Newport, NC 34N47-76W53" },
	{ "USA", "", "WYFR", "Okeechobee, FL 27N28-80W56" },
	{ "USA", "", "WWBS", "Macon, GA 32N50-83W38" },
	{ "USA", "", "WWCR", "Nashville, TN 36N13-86W54" },
	{ "USA", "", "WWRB", "Manchester / Morrison, TN 35N29-86W02" },
	{ "UZB", "", "", "Tashkent 41N13-69E09" },
	{ "VTN", "", "", "Sontay 21N12-105E22" },
	{ "VTN", "d", "", "Daclac 12N41-108E31" },
	{ "VTN", "h", "", "Hanoi-Metri 20N59-105E52" },
	{ "VTN", "x", "", "Xuan Mai 20N43-105E33" },
	{ "YEM", "a", "", "Al Hiswah/Aden 12N50-45E02" },
	{ "YEM", "s", "", "Sanaa 15N22-44E11" },
	{ 0, 0, 0, 0}
	};


void CDRMSchedule::ReadCSVFile(FILE* pFile)
{
	const int	iMaxLenRow = 1024;
	char		cRow[iMaxLenRow];

	size_t i;
	map<string,string> l;
	for(i=0; langs[i].code; i++)
	{
		l[langs[i].code] = langs[i].lang;
	}

	map<string,string> c;
	for(i=0; countries[i].code; i++)
	{
		c[countries[i].code] = countries[i].country;
	}

	map<string,string> t;
	for(i=0; targets[i].code; i++)
	{
		string code = targets[i].code;
		string v = targets[i].target;
		string C = "C"; string c = "Central ";
		string N = "N"; string n = "North ";
		string S = "S"; string s = "South ";
		string E = "E"; string e = "East ";
		string W = "W"; string w = "West ";
		t[code] = v;
		t[C+code] = c+v;
		t[N+code] = n+v;
		t[S+code] = s+v;
		t[E+code] = e+v;
		t[W+code] = w+v;
	}
	map<string, map<string,string> > tx;
	for(i=0; stations[i].country; i++)
	{
		tx[stations[i].country][stations[i].mark] = stations[i].site;
	}

	do {
		CStationsItem StationsItem;
		string s;
		map<string,string>::const_iterator m;

		fgets(cRow, iMaxLenRow, pFile);
		vector<string> fields;
		stringstream ss(cRow);
		do {
			getline(ss, s, ';');
			fields.push_back(s);
		} while(!ss.eof());

		ss.str(fields[0]);
		ss >> StationsItem.iFreq;

		QStringList times = QStringList::split("-", fields[1].c_str());
		StationsItem.SetStartTimeNum(times[0].toInt());
		StationsItem.SetStopTimeNum(times[1].toInt());

		if(fields[2].length()>0)
		{
			stringstream ss(fields[2]);
			char c;
			enum Days { Sunday=0, Monday=1, Tuesday=2, Wednesday=3,
						Thursday=4, Friday=5, Saturday=6 };
			Days first=Sunday, last=Sunday;
			enum { no, in, done } range_state = no;
			// Days[SMTWTFS]=1111111
			string strDays = "0000000";
			while(!ss.eof())
			{
				ss >> c;
				switch(c)
				{
					case '-':
						range_state = in;
						break;
					case 'M':
						ss >> c;
						last = Monday;
						break;
					case 'T':
						ss >> c;
						last = (c=='u')?Tuesday:Thursday;
						break;
					case 'W':
						ss >> c;
						last = Wednesday;
						break;
					case 'F':
						ss >> c;
						last = Friday;
						break;
					case 'S':
						ss >> c;
						last = (c=='u')?Sunday:Saturday;
						break;
				}
				switch(range_state)
				{
					case no:
						strDays[last] = '1';
						break;
					case in:
						first = last;
						range_state = done;
						break;
					case done:
						if(first<last)
						{
							for(int d=first; d<=last; d++)
								strDays[d] = '1';
						}
						range_state = no;
						break;
				}
			}
			StationsItem.SetDaysFlagString(strDays);
		}
		else
			StationsItem.SetDaysFlagString("1111111");

		//StationsItem.rPower = 0.0;
//0   ;1        ;2    ;3  ;4               ;5;6;7;8;9;10
//1170;1600-1700;Mo-Fr;USA;Voice of America;E; ; ;0; ;
		string homecountry;
		if(fields.size()>3)
		{
			homecountry = fields[3];
			m = c.find(homecountry);
			if(m != c.end())
				StationsItem.strCountry = m->second;
			else
				StationsItem.strCountry = homecountry;
		}

		if(fields.size()>4)
			StationsItem.strName = fields[4];

		if(fields.size()>5)
		{
			m = l.find(fields[5]);
			if(m != l.end())
				StationsItem.strLanguage = m->second;
		}

		if(fields.size()>6)
		{
			s = fields[6];
			m = t.find(s);
			if(m != t.end())
			{
				StationsItem.strTarget = m->second;
			}
			else
			{
				m = c.find(s);
				if(m != c.end())
					StationsItem.strTarget = m->second;
				else
					StationsItem.strTarget = s;
			}
		}
		string country;
		string stn;
		if(fields.size()>7)
		{
			s = fields[7];
			if(s=="") // unknown or main Tx site of the home country
			{
				country = homecountry;
			}
			else
			{
				size_t i=0;
				s += '-';
				if(s[0]=='/') // transmitted from another country
					i++;
				string a,b;
				while(s[i]!='-')
					a += s[i++];
				i++;
				if(i<s.length())
					while(s[i]!='-')
						b += s[i++];
				if(s[0]=='/')
				{
					country = a;
					stn = b;
				}
				else
				{
					if(a.length()==3)
					{
						country = a;
						stn = b;
					}
					else
					{
						country = homecountry;
						stn = a;
					}
				}
			}
		}
		else
		{
			country = homecountry;
		}
		map<string,string>& tx_sites = tx[country];
		map<string,string>::const_iterator txs = tx_sites.find(stn);
		if(txs==tx_sites.end())
		{
			StationsItem.strSite = s;
			cout << "[" << s << "] [" << country << "] [" << stn << "]" << endl;
		}
		else
		{
			StationsItem.strSite = txs->second;
		}

		/* Add new item in table */
		StationsTable.push_back(StationsItem);

	} while(!feof(pFile));
}

CDRMSchedule::StationState CDRMSchedule::CheckState(const int iPos)
{
	/* Get system time */
	time_t ltime;
	time(&ltime);

	if (IsActive(iPos, ltime) == TRUE)
	{
		/* Check if the station soon will be inactive */
		if (IsActive(iPos, ltime + NUM_SECONDS_SOON_INACTIVE) == TRUE)
			return IS_ACTIVE;
		else
			return IS_SOON_INACTIVE;
	}
	else
	{
		/* Station is not active, check preview condition */
		if (iSecondsPreview > 0)
		{
			if (IsActive(iPos, ltime + iSecondsPreview) == TRUE)
				return IS_PREVIEW;
			else
				return IS_INACTIVE;
		}
		else
			return IS_INACTIVE;
	}
}

_BOOLEAN CDRMSchedule::IsActive(const int iPos, const time_t ltime)
{
	/* Calculate time in UTC */
	struct tm* gmtCur = gmtime(&ltime);
	const time_t lCurTime = mktime(gmtCur);

	/* Get stop time */
	struct tm* gmtStop = gmtime(&ltime);
	gmtStop->tm_hour = StationsTable[iPos].iStopHour;
	gmtStop->tm_min = StationsTable[iPos].iStopMinute;
	const time_t lStopTime = mktime(gmtStop);

	/* Get start time */
	struct tm* gmtStart = gmtime(&ltime);
	gmtStart->tm_hour = StationsTable[iPos].iStartHour;
	gmtStart->tm_min = StationsTable[iPos].iStartMinute;
	const time_t lStartTime = mktime(gmtStart);

	/* Check, if stop time is on next day */
	_BOOLEAN bSecondDay = FALSE;
	if (lStopTime < lStartTime)
	{
		/* Check, if we are at the first or the second day right now */
		if (lCurTime < lStopTime)
		{
			/* Second day. Increase day count */
			gmtCur->tm_wday++;

			/* Check that value is valid (range 0 - 6) */
			if (gmtCur->tm_wday > 6)
				gmtCur->tm_wday = 0;

			/* Set flag */
			bSecondDay = TRUE;
		}
	}

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0). "strDaysFlags" are coded with
	   pseudo binary representation. A one signalls that day is active. The most
	   significant 1 is the sunday, then followed the monday and so on. */
	if ((StationsTable[iPos].strDaysFlags[gmtCur->tm_wday] ==
		CHR_ACTIVE_DAY_MARKER) ||
		/* Check also for special case: days are 0000000. This is reserved for
		   DRM test transmissions or irregular transmissions. We define here
		   that these stations are transmitting every day */
		(StationsTable[iPos].strDaysFlags == FLAG_STR_IRREGULAR_TRANSM))
	{
		/* Check time interval */
		if (lStopTime > lStartTime)
		{
			if ((lCurTime >= lStartTime) && (lCurTime < lStopTime))
				return TRUE;
		}
		else
		{
			if (bSecondDay == FALSE)
			{
				/* First day. Only check if we are after start time */
				if (lCurTime >= lStartTime)
					return TRUE;
			}
			else
			{
				/* Second day. Only check if we are before stop time */
				if (lCurTime < lStopTime)
					return TRUE;
			}
		}
	}

	return FALSE;
}

void CStationsItem::SetDaysFlagString(const string strNewDaysFlags)
{
	/* Set internal "days flag" string and "show days" string */
	strDaysFlags = strNewDaysFlags;
	strDaysShow = "";

	/* Init days string vector */
	const QString strDayDef [] =
	{
		QObject::tr("Sun"),
		QObject::tr("Mon"),
		QObject::tr("Tue"),
		QObject::tr("Wed"),
		QObject::tr("Thu"),
		QObject::tr("Fri"),
		QObject::tr("Sat")
	};

	/* First test for day constellations which allow to apply special names */
	if (strDaysFlags == FLAG_STR_IRREGULAR_TRANSM)
		strDaysShow = QObject::tr("irregular").latin1();
	else if (strDaysFlags == "1111111")
		strDaysShow = QObject::tr("daily").latin1();
	else if (strDaysFlags == "1111100")
		strDaysShow = QObject::tr("from Sun to Thu").latin1();
	else if (strDaysFlags == "1111110")
		strDaysShow = QObject::tr("from Sun to Fri").latin1();
	else if (strDaysFlags == "0111110")
		strDaysShow = QObject::tr("from Mon to Fri").latin1();
	else if (strDaysFlags == "0111111")
		strDaysShow = QObject::tr("from Mon to Sat").latin1();
	else
	{
		/* No special name could be applied, just list all active days */
		for (int i = 0; i < 7; i++)
		{
			/* Check if day is active */
			if (strDaysFlags[i] == CHR_ACTIVE_DAY_MARKER)
			{
				/* Set commas in between the days, to not set a comma at
				   the beginning */
				if (strDaysShow != "")
					strDaysShow += ",";

				/* Add current day */
				strDaysShow += strDayDef[i].latin1();
			}
		}
	}
}

StationsDlg::StationsDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f) :
	CStationsDlgBase(parent, name, modal, f),
	DRMReceiver(NDRMR),
	Settings(NSettings),
	bReInitOnFrequencyChange(FALSE), vecpListItems(0),
	iTunedFrequency(-1), bFrequencySetFromReceiver(FALSE)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Stations Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	/* Define size of the bitmaps */
	const int iXSize = 13;
	const int iYSize = 13;

	/* Create bitmaps */
	BitmCubeGreen.resize(iXSize, iYSize);
	BitmCubeGreen.fill(QColor(0, 255, 0));
	BitmCubeYellow.resize(iXSize, iYSize);
	BitmCubeYellow.fill(QColor(255, 255, 0));
	BitmCubeRed.resize(iXSize, iYSize);
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeOrange.resize(iXSize, iYSize);
	BitmCubeOrange.fill(QColor(255, 128, 0));
	BitmCubePink.resize(iXSize, iYSize);
	BitmCubePink.fill(QColor(255, 128, 128));

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(0, tr("Station Name"));
	ListViewStations->addColumn(tr("Time [UTC]"));
	ListViewStations->addColumn(tr("Frequency [kHz]"));
	ListViewStations->addColumn(tr("Target"));
	ListViewStations->addColumn(tr("Power [kW]"));
	ListViewStations->addColumn(tr("Country"));
	ListViewStations->addColumn(tr("Site"));
	ListViewStations->addColumn(tr("Language"));
	ListViewStations->addColumn(tr("Days"));

	/* Set right alignment for numeric columns */
	ListViewStations->setColumnAlignment(2, Qt::AlignRight);
	ListViewStations->setColumnAlignment(4, Qt::AlignRight);

	/* Set up frequency selector control (QWTCounter control) */
	QwtCounterFrequency->setRange(0.0, 30000.0, 1.0);
	QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
	QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

	/* Init UTC time shown with a label control */
	SetUTCTimeLabel();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	pViewMenu = new QPopupMenu(this);
	CHECK_PTR(pViewMenu);
	pViewMenu->insertItem(tr("Show &only active stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem(tr("Show &all stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Set stations in list view which are active right now */
	bShowAll = FALSE;
	pViewMenu->setItemChecked(0, TRUE);

	/* Stations Preview menu ------------------------------------------------ */
	pPreviewMenu = new QPopupMenu(this);
	CHECK_PTR(pPreviewMenu);
	pPreviewMenu->insertItem(tr("&Disabled"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 0);
	pPreviewMenu->insertItem(tr("&5 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 1);
	pPreviewMenu->insertItem(tr("&15 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 2);
	pPreviewMenu->insertItem(tr("&30 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 3);

	/* Set stations preview */
	/* Retrieve the setting saved into the .ini file */
	switch (Settings.Get("Stations Dialog", "preview", NUM_SECONDS_PREV_5MIN))
	{
	case NUM_SECONDS_PREV_5MIN:
		pPreviewMenu->setItemChecked(1, TRUE);
		break;

	case NUM_SECONDS_PREV_15MIN:
		pPreviewMenu->setItemChecked(2, TRUE);
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case NUM_SECONDS_PREV_30MIN:
		pPreviewMenu->setItemChecked(3, TRUE);
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0, also takes care of out of value parameters */
		pPreviewMenu->setItemChecked(0, TRUE);
		DRMSchedule.SetSecondsPreview(0);
		break;
	}

	pViewMenu->insertSeparator();
	pViewMenu->insertItem(tr("Stations &preview"),pPreviewMenu);

	SetStationsView();

	/* Update menu ---------------------------------------------------------- */
	pUpdateMenu = new QPopupMenu(this);
	CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem(tr("&Get Update..."), this, SLOT(OnGetUpdate()), 0, 0);

	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), pViewMenu);
	pMenu->insertItem(tr("&Update"), pUpdateMenu); /* String "Update" used below */
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CStationsDlgBaseLayout->setMenuBar(pMenu);


	/* Register the network protocols (ftp, http). Needed for the schedule download */
	QNetworkProtocol::registerNetworkProtocol("ftp", new QNetworkProtocolFactory<QFtp>);
#if QT_VERSION >= 0x030000
	QNetworkProtocol::registerNetworkProtocol("http", new QNetworkProtocolFactory<QHttp>);
#endif


	/* Connections ---------------------------------------------------------- */
	connect(&TimerList, SIGNAL(timeout()),
		this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()),
		this, SLOT(OnTimerUTCLabel()));
	connect(&TimerSMeter, SIGNAL(timeout()),
		this, SLOT(OnTimerSMeter()));

	TimerList.stop();
	TimerUTCLabel.stop();
	TimerSMeter.stop();

	connect(ListViewStations, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));

	connect(ListViewStations->header(), SIGNAL(clicked(int)),
		this, SLOT(OnHeaderClicked(int)));

	connect(&UrlUpdateSchedule, SIGNAL(finished(QNetworkOperation*)),
		this, SLOT(OnUrlFinished(QNetworkOperation*)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));

	ProgrSigStrength->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
	ProgrSigStrength->setOrientation(QwtThermo::Horizontal, QwtThermo::Top);
	ProgrSigStrength->setAlarmLevel(S_METER_THERMO_ALARM);
	ProgrSigStrength->setAlarmColor(QColor(255, 0, 0));
	ProgrSigStrength->setScale(S_METER_THERMO_MIN, S_METER_THERMO_MAX, 10.0);
}

StationsDlg::~StationsDlg()
{
}

void StationsDlg::SetUTCTimeLabel()
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	/* Generate time in format "UTC 12:00" */
	QString strUTCTime = QString().sprintf("%02d:%02d UTC",
		gmtCur->tm_hour, gmtCur->tm_min);

	/* Only apply if time label does not show the correct time */
	if (TextLabelUTCTime->text().compare(strUTCTime))
		TextLabelUTCTime->setText(strUTCTime);
}

void StationsDlg::OnShowStationsMenu(int iID)
{
	/* Show only active stations if ID is 0, else show all */
	if (iID == 0)
	{
		bShowAll = FALSE;
		/* clear all and reload. If the list is too big this increase the performance */
		ClearStationsView();
	}
	else
		bShowAll = TRUE;

	/* Update list view */
	SetStationsView();

	/* Taking care of checks in the menu */
	pViewMenu->setItemChecked(0, 0 == iID);
	pViewMenu->setItemChecked(1, 1 == iID);
}

void StationsDlg::OnShowPreviewMenu(int iID)
{
	switch (iID)
	{
	case 1:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
		break;

	case 2:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case 3:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0: */
		DRMSchedule.SetSecondsPreview(0);
		break;
	}

	/* Update list view */
	SetStationsView();

	/* Taking care of checks in the menu */
	pPreviewMenu->setItemChecked(0, 0 == iID);
	pPreviewMenu->setItemChecked(1, 1 == iID);
	pPreviewMenu->setItemChecked(2, 2 == iID);
	pPreviewMenu->setItemChecked(3, 3 == iID);
}

void StationsDlg::AddUpdateDateTime()
{
/*
	Set time and date of current DRM schedule in the menu text for
	querying a new schedule (online schedule update).
	If no schedule file exists, do not show any time and date.
*/
	/* make sure we check the correct file (DRM or AM schedule) */
	QString strFile;
	switch (DRMSchedule.GetSchedMode())
	{
		case CDRMSchedule::SM_DRM:
			strFile = DRMSCHEDULE_INI_FILE_NAME;
			pUpdateMenu->setItemEnabled(0, TRUE);
			break;

		case CDRMSchedule::SM_ANALOG:
			strFile = AMSCHEDULE_INI_FILE_NAME;
			pUpdateMenu->setItemEnabled(0, TRUE);
			break;
	}

	/* init with empty string in case there is no schedule file */
	QString s = "";

	/* get time and date information */
	QFileInfo f = QFileInfo(strFile);
	if (f.exists()) /* make sure the DRM schedule file exists */
	{
		/* use QT-type of data string for displaying */
		s = tr(" (last update: ")
			+ f.lastModified().date().toString() + ")";
	}

	pUpdateMenu->changeItem(tr("&Get Update") + s + "...", 0);
}

void StationsDlg::OnGetUpdate()
{
	switch (DRMSchedule.GetSchedMode())
	{
		case CDRMSchedule::SM_DRM:
			if (QMessageBox::information(this, tr("Dream Schedule Update"),
				tr("Dream tries to download the newest DRM schedule\nfrom "
				"www.drm-dx.de (powered by Klaus Schneider).\nYour computer "
				"must be connected to the internet.\n\nThe current file "
				"DRMSchedule.ini will be overwritten.\nDo you want to "
				"continue?"),
				QMessageBox::Yes, QMessageBox::No) == 3 /* Yes */)
			{
				/* Try to download the current schedule. Copy the file to the
		   		current working directory (which is "QDir().absFilePath(NULL)") */
				UrlUpdateSchedule.copy(QString(DRM_SCHEDULE_UPDATE_FILE),
					QString(QDir().absFilePath(NULL)));
			}
			break;

		case CDRMSchedule::SM_ANALOG:
			QDate d = QDate::currentDate();
#if QT_VERSION >= 0x030000
			int wk = d.weekNumber();
#else
			int wk = d.dayOfYear()/7;
#endif
			int yr = d.year();
			QString y,w;
			if(13 <= wk)
			{
				w = "b";
				y = QString::number(yr-1);
			}
			else if(wk <= 43)
			{
				w = "a";
				y = QString::number(yr);
			}
			else
			{
				w = "b";
				y = QString::number(yr);
			}
#if QT_VERSION >= 0x030000
			QString path = QString(AM_SCHEDULE_UPDATE_FILE).arg(w, y.right(2));
			if (QMessageBox::information(this, tr("Dream Schedule Update"),
				tr("Dream tries to download the newest EIBI AM schedule.\n"
				"  Your computer must be connected to the internet.\n\n"
				"The current file AMSchedule.ini will be overwritten.\n"
				"Do you want to continue?"),
				QMessageBox::Yes, QMessageBox::No) == 3 /* Yes */)
			{
				/* Try to download the current schedule. Copy the file to the
		   		current working directory (which is "QDir().absFilePath(NULL)") */
				UrlUpdateSchedule.copy(path,
					QString(QDir().absFilePath(NULL))+"/AMSchedule.ini", FALSE, FALSE);
			}
#else
			QString path = QString(AM_SCHEDULE_UPDATE_FILE).arg(w).arg(y.right(2));
			QMessageBox::information(this, tr("Dream Schedule Update"),
				tr("This version of Dream can't download the AM schedule.\n"
				"Download the file and save as AMSchedule.ini.\n"
				"The url for the file is ")+path); 
#endif
			break;
	}

}

void StationsDlg::OnUrlFinished(QNetworkOperation* pNetwOp)
{
	/* Check that pointer points to valid object */
	if (pNetwOp)
	{
		if (pNetwOp->state() == QNetworkProtocol::StFailed)
		{
			/* Something went wrong -> stop all network operations */
			UrlUpdateSchedule.stop();

			/* Notify the user of the failure */
			switch (DRMSchedule.GetSchedMode())
			{
				case CDRMSchedule::SM_DRM:
					QMessageBox::information(this, "Dream",
						tr("Update failed. The following things may caused the "
						"failure:\n"
						"\t- the internet connection was not set up properly\n"
						"\t- the server www.drm-dx.de is currently not available\n"
						"\t- the file 'DRMSchedule.ini' could not be written"),
						QMessageBox::Ok);
					break;
				case CDRMSchedule::SM_ANALOG:
					QMessageBox::information(this, "Dream",
						tr("Update failed. The following things may caused the failure:\n"
						"\t- the internet connection was not set up properly\n"
						"\t- the server www.susi-und-strolch.de is currently not available\n"
						"\t- the file 'AMSchedule.ini' could not be written"),
						QMessageBox::Ok);
					break;
			}
		}

		/* We are interested in the state of the final put function */
		if (pNetwOp->operation() == QNetworkProtocol::OpPut)
		{
			if (pNetwOp->state() == QNetworkProtocol::StDone)
			{
				/* Notify the user that update was successful */
#ifdef _WIN32
				QMessageBox::warning(this, "Dream", tr("Update successful.\n"
					"Due to network problems with the Windows version of QT, "
					"the Dream software must be restarted after a schedule "
					"update.\nPlease exit Dream now."),
					tr("Ok"));
#else
				QMessageBox::information(this, "Dream",
					tr("Update successful."), QMessageBox::Ok);
#endif

				/* Read updated ini-file */
				LoadSchedule(DRMSchedule.GetSchedMode());
			}
		}
	}
}

void StationsDlg::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timers */
	TimerList.stop();
	TimerUTCLabel.stop();
	TimerSMeter.stop();
	EnableSMeter(FALSE);

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	CWinGeom c;
	c.iXPos = WinGeom.x();
	c.iYPos = WinGeom.y();
	c.iHSize = WinGeom.height();
	c.iWSize = WinGeom.width();
	Settings.Put("Stations Dialog", c);

	/* Store preview settings */
	Settings.Put("Stations Dialog", "preview", DRMSchedule.GetSecondsPreview());

	/* Store sort settings */
	switch (DRMSchedule.GetSchedMode())
	{
	case CDRMSchedule::SM_DRM:
		Settings.Put("Stations Dialog", "sortcolumndrm", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendingdrm", bCurrentSortAscending);
		break;

	case CDRMSchedule::SM_ANALOG:
		Settings.Put("Stations Dialog", "sortcolumnanalog", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendinganalog", bCurrentSortAscending);
		break;
	}
}

void StationsDlg::showEvent(QShowEvent*)
{

	/* Init with current setting in log file */
	iTunedFrequency = DRMReceiver.GetFrequency();
	bFrequencySetFromReceiver = TRUE;
	QwtCounterFrequency->setValue(iTunedFrequency);

	/* Load the schedule if necessary */
	if (DRMSchedule.GetStationNumber() == 0)
		LoadSchedule(DRMSchedule.GetSchedMode());

	/* If number of stations is zero, we assume that the ini file is missing */
	if (DRMSchedule.GetStationNumber() == 0)
	{
		if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
		{
			QMessageBox::information(this, "Dream", tr("The file "
				DRMSCHEDULE_INI_FILE_NAME
				" could not be found or contains no data.\nNo "
				"stations can be displayed.\nTry to download this file by "
				"using the 'Update' menu."), QMessageBox::Ok);
		}
		else
		{
			QMessageBox::information(this, "Dream", tr("The file "
				AMSCHEDULE_INI_FILE_NAME
				" could not be found or contains no data.\nNo "
				"stations can be displayed."), QMessageBox::Ok);
		}
	}

	/* Update window */
	OnTimerUTCLabel();
	OnTimerList();

	/* Activate real-time timer when window is shown */
	TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);

	/* add last update information on menu item */
	AddUpdateDateTime();
}

void StationsDlg::OnTimerList()
{
	/* Update list view */
	SetStationsView();

	/* S-meter settings */
	_BOOLEAN bSMeter = DRMReceiver.GetEnableSMeter();
	if(bSMeter)
		TimerSMeter.start(GUI_TIMER_S_METER);
	else
		TimerSMeter.stop();
	EnableSMeter(bSMeter);
}

QString MyListViewItem::key(int column, bool ascending) const
{
	/* Reimplement "key()" function to get correct sorting behaviour */
	if ((column == 2) || (column == 4))
	{
		/* These columns are filled with numbers. Some items may have numbers
		   after the comma, therefore multiply with 10000 (which moves the
		   numbers in front of the comma). Afterwards append zeros at the
		   beginning so that positive integer numbers are sorted correctly */
		return QString(QString().setNum((long int)
			(text(column).toFloat() * 10000.0))).rightJustify(20, '0');
	}
    else
		return QListViewItem::key(column, ascending);
}

void StationsDlg::SetSortSettings(const CDRMSchedule::ESchedMode eNewSchM)
{
	/* Store the current sort settings before switching */
	if (eNewSchM != DRMSchedule.GetSchedMode())
	{
		switch (DRMSchedule.GetSchedMode())
		{
		case CDRMSchedule::SM_DRM:
			Settings.Put("Stations Dialog", "sortcolumndrm", iCurrentSortColumn);
			Settings.Put("Stations Dialog", "sortascendingdrm", bCurrentSortAscending);
			break;

		case CDRMSchedule::SM_ANALOG:
			Settings.Put("Stations Dialog", "sortcolumnanalog", iCurrentSortColumn);
			Settings.Put("Stations Dialog", "sortascendinganalog", bCurrentSortAscending);
			break;
		}
	}

	/* Set sorting behaviour of the list */
	switch (eNewSchM)
	{
	case CDRMSchedule::SM_DRM:
		iCurrentSortColumn = Settings.Get("Stations Dialog", "sortcolumndrm", 0);
		bCurrentSortAscending = Settings.Get("Stations Dialog", "sortascendingdrm", TRUE);
		break;

	case CDRMSchedule::SM_ANALOG:
		iCurrentSortColumn = Settings.Get("Stations Dialog", "sortcolumnanalog", 0);
		bCurrentSortAscending = Settings.Get("Stations Dialog", "sortascendinganalog", TRUE);
		break;
	}
	ListViewStations->setSorting(iCurrentSortColumn, bCurrentSortAscending);
}

void StationsDlg::SetCurrentSchedule(const CDRMSchedule::ESchedMode eNewSchM)
{
	SetSortSettings(eNewSchM);

	/* Save new mode */
	DRMSchedule.SetSchedMode(eNewSchM);
}

void StationsDlg::LoadSchedule(CDRMSchedule::ESchedMode eNewSchM)
{
	SetSortSettings(eNewSchM);

	ClearStationsView();

	/* Read initialization file */
	DRMSchedule.ReadStatTabFromFile(eNewSchM);

	/* Update list view */
	SetStationsView();

	/* Add last update information on menu item if the dialog is visible */
	if (this->isVisible())
		AddUpdateDateTime();
}

void StationsDlg::ClearStationsView()
{
	/* Delete all old list view items (it is important that the vector
	   "vecpListItems" was initialized to 0 at creation of the global object
	   otherwise this may cause an segmentation fault) */
	ListItemsMutex.lock();
	ListViewStations->clear();
	/*
	for (size_t i = 0; i < vecpListItems.size(); i++)
	{
		if (vecpListItems[i] != NULL)
			delete vecpListItems[i];
	}
	*/
	vecpListItems.clear();
	ListItemsMutex.unlock();
}

void StationsDlg::SetStationsView()
{
	size_t i;
	/* Stop the timer and disable the list */
	TimerList.stop();

	const _BOOLEAN bListFocus = ListViewStations->hasFocus();

	ListViewStations->setUpdatesEnabled(FALSE);
	ListViewStations->setEnabled(FALSE);

	/* Set lock because of list view items. These items could be changed
	   by another thread */
	ListItemsMutex.lock();

	const size_t iNumStations = DRMSchedule.GetStationNumber();
	_BOOLEAN bListHastChanged = FALSE;

	/* if the list got smaller, we need to free some memory */
	for (i = iNumStations; i < vecpListItems.size(); i++)
	{
		if (vecpListItems[i] != NULL)
			delete vecpListItems[i];
	}
	/* resize will leave all existing elements alone and add
	 * nulls in case the list needed to get bigger
	 */
	vecpListItems.resize(iNumStations, (MyListViewItem*) NULL);

	/* Add new item for each station in list view */
	for (i = 0; i < iNumStations; i++)
	{
		CDRMSchedule::StationState iState = DRMSchedule.CheckState(i);

		if (!((bShowAll == FALSE) &&
			(iState == CDRMSchedule::IS_INACTIVE)))
		{
			/* Only insert item if it is not already in the list */
			if (vecpListItems[i] == NULL)
			{
				/* Get power of the station. We have to do a special treatment
				   here, because we want to avoid having a "0" in the list when
				   a "?" was in the schedule-ini-file */
				const _REAL rPower = DRMSchedule.GetItem(i).rPower;

				QString strPower;
				if (rPower == (_REAL) 0.0)
					strPower = "?";
				else
					strPower.setNum(rPower);

				/* Generate new list item with all necessary column entries */
				vecpListItems[i] = new MyListViewItem(ListViewStations,
					DRMSchedule.GetItem(i).strName.c_str()     /* name */,
					QString().sprintf("%04d-%04d",
					DRMSchedule.GetItem(i).GetStartTimeNum(),
					DRMSchedule.GetItem(i).GetStopTimeNum())   /* time */,
					QString().setNum(DRMSchedule.GetItem(i).iFreq) /* freq. */,
					DRMSchedule.GetItem(i).strTarget.c_str()   /* target */,
					strPower                                   /* power */,
					DRMSchedule.GetItem(i).strCountry.c_str()  /* country */,
					DRMSchedule.GetItem(i).strSite.c_str()     /* site */,
					DRMSchedule.GetItem(i).strLanguage.c_str() /* language */);

				/* Show list of days */
				vecpListItems[i]->setText(8,
					DRMSchedule.GetItem(i).strDaysShow.c_str());

				/* Insert this new item in list. The item object is destroyed by
				   the list view control when this is destroyed */
				ListViewStations->insertItem(vecpListItems[i]);

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}

			/* Check, if station is currently transmitting. If yes, set
			   special pixmap */
			switch (iState)
			{
				case CDRMSchedule::IS_ACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubeGreen);
					break;
				case CDRMSchedule::IS_PREVIEW:
					vecpListItems[i]->setPixmap(0, BitmCubeOrange);
					break;
				case CDRMSchedule::IS_SOON_INACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubePink);
					break;
				case CDRMSchedule::IS_INACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubeRed);
					break;
				default:
					vecpListItems[i]->setPixmap(0, BitmCubeRed);
					break;
			}
		}
		else
		{
			/* Delete this item since it is not used anymore */
			if (vecpListItems[i] != NULL)
			{
				/* If one deletes a item in QT list view, it is
				   automaticall removed from the list and the list gets
				   repainted */
				delete vecpListItems[i];

				/* Reset pointer so we can distinguish if it is used or not */
				vecpListItems[i] = NULL;

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}
		}
	}

	/* Sort the list if items have changed */
	if (bListHastChanged == TRUE)
		ListViewStations->sort();

	ListItemsMutex.unlock();

	/* Start the timer and enable the list */
	ListViewStations->setUpdatesEnabled(TRUE);
	ListViewStations->setEnabled(TRUE);

	/* to update the scrollbars */
	ListViewStations->triggerUpdate();

	if (bListFocus == TRUE)
		ListViewStations->setFocus();

	TimerList.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnFreqCntNewValue(double dVal)
{
	/* Set frequency to front-end */
	if(bFrequencySetFromReceiver)
	{
		bFrequencySetFromReceiver = FALSE;
	}
	else
	{
		iTunedFrequency = (int) dVal;
		DRMReceiver.SetFrequency(iTunedFrequency);
	}
	QListViewItem* item = ListViewStations->selectedItem();
	if(item)
	{
		if(QString(item->text(2)).toInt() != (int) dVal)
			ListViewStations->clearSelection();
	}
}

void StationsDlg::OnHeaderClicked(int c)
{
	/* Store the "direction" of sorting */
	if (iCurrentSortColumn == c)
		bCurrentSortAscending = !bCurrentSortAscending;
	else
		bCurrentSortAscending = TRUE;

	iCurrentSortColumn = c;
}

void StationsDlg::OnListItemClicked(QListViewItem* item)
{
	/* Check that it is a valid item (!= 0) */
	if (item)
	{
		/* Third text of list view item is frequency -> text(2)
		   Set value in frequency counter control QWT. Setting this parameter
		   will emit a "value changed" signal which sets the new frequency.
		   Therefore, here is no call to "SetFrequency()" needed.*/
		QwtCounterFrequency->setValue(QString(item->text(2)).toInt());

		/* If the mode has changed re-initialise the receiver */
		ERecMode eCurrentMode = DRMReceiver.GetReceiverMode();

		/* if "bReInitOnFrequencyChange" is not true, initiate a reinit when
		 schedule mode is different from receiver mode */
		switch (DRMSchedule.GetSchedMode())
		{
		case CDRMSchedule::SM_DRM:
			if (eCurrentMode != RM_DRM)
				DRMReceiver.SetReceiverMode(RM_DRM);
			if (bReInitOnFrequencyChange)
				DRMReceiver.RequestNewAcquisition();
			break;

		case CDRMSchedule::SM_ANALOG:
			if (eCurrentMode != RM_AM)
				DRMReceiver.SetReceiverMode(RM_AM);
			if (bReInitOnFrequencyChange)
				DRMReceiver.RequestNewAcquisition();
			break;
		}
	}
}

void StationsDlg::OnTimerSMeter()
{
	EnableSMeter(TRUE);

	/* Update frequency edit control
	 * frequency could be changed by evaluation dialog
	 * or RSCI
	 */
	int iFrequency = DRMReceiver.GetFrequency();
	if(iFrequency != iTunedFrequency)
	{
		iTunedFrequency = iFrequency;
		QwtCounterFrequency->setValue(iFrequency);
		bFrequencySetFromReceiver = TRUE;
	}
}

void StationsDlg::EnableSMeter(const _BOOLEAN bStatus)
{
	/* Need both, GUI "enabled" and signal strength valid before s-meter is used */
	_REAL rCurSigStr;
	_BOOLEAN bValid = DRMReceiver.GetSignalStrength(rCurSigStr);

	if((bStatus == TRUE) && (bValid == TRUE))
	{
		/* Init progress bar for input s-meter */
		ProgrSigStrength->setAlarmEnabled(TRUE);
		ProgrSigStrength->setValue(rCurSigStr);
		ProgrSigStrength->setFillColor(QColor(0, 190, 0));

		ProgrSigStrength->setEnabled(TRUE);
		TextLabelSMeter->setEnabled(TRUE);
		ProgrSigStrength->show();
		TextLabelSMeter->show();
	}
	else
	{
		/* Set s-meter control in "disabled" status */
		ProgrSigStrength->setAlarmEnabled(FALSE);
		ProgrSigStrength->setValue(S_METER_THERMO_MAX);
		ProgrSigStrength->setFillColor(palette().disabled().light());

		ProgrSigStrength->setEnabled(FALSE);
		TextLabelSMeter->setEnabled(FALSE);
		ProgrSigStrength->hide();
		TextLabelSMeter->hide();
	}
}

void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	QWhatsThis::add(ListViewStations,
		tr("<b>Stations List:</b> In the stations list "
		"view all DRM stations which are stored in the DRMSchedule.ini file "
		"are shown. It is possible to show only active stations by changing a "
		"setting in the 'view' menu. The color of the cube on the left of a "
		"menu item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline, "
		"a pink cube shown that the transmission soon will be offline.<br>"
		"If the stations preview is active an orange box shows the stations "
		"that will be active.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column.<br>By clicking on a menu item, a remote front-end can "
		"be automatically switched to the current frequency and the "
		"Dream software is reset to a new acquisition (to speed up the "
		"synchronization process). Also, the log-file frequency edit "
		"is automatically updated."));

	/* Frequency Counter */
	QWhatsThis::add(QwtCounterFrequency,
		tr("<b>Frequency Counter:</b> The current frequency "
		"value can be changed by using this counter. The tuning steps are "
		"100 kHz for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically."));

	/* UTC time label */
	QWhatsThis::add(TextLabelUTCTime,
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is nearly the same as Greenwich Mean Time "
		"(GMT)."));

#ifdef HAVE_LIBHAMLIB
	/* S-meter */
	const QString strSMeter =
		tr("<b>Signal-Meter:</b> Shows the signal strength "
		"level in dB relative to S9.<br>Note that not all "
		"front-ends controlled by hamlib support this feature. If the s-meter "
		"is not available, the controls are disabled.");

	QWhatsThis::add(TextLabelSMeter, strSMeter);
	QWhatsThis::add(ProgrSigStrength, strSMeter);
#endif
}
