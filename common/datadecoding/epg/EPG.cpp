/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide class
 *
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

#include "EPG.h"
#include "epgutil.h"
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>

EPG::EPG ()
{
	genres["1"] = "Audio-Video";
	genres["1.0"] = "Proprietary";
	genres["1.1"] = "PlayRecording";
	genres["1.1.1"] = "Pure entertainment";
	genres["1.1.2"] = "Informative entertainment";
	genres["1.10"] = "Mute";
	genres["1.11"] = "VolumeUp";
	genres["1.12"] = "VolumeDown";
	genres["1.13"] = "Loop/Repeat";
	genres["1.14"] = "Shuffle";
	genres["1.15"] = "SkipToStart";
	genres["1.16"] = "SkipToEnd";
	genres["1.17"] = "CopyCD";
	genres["1.2"] = "PlayStream";
	genres["1.2.1"] = "Government";
	genres["1.2.2"] = "Pure information";
	genres["1.2.3"] = "Infotainment";
	genres["1.2.4"] = "Advice";
	genres["1.3"] = "Record";
	genres["1.3.1"] = "School Programmes";
	genres["1.3.1.1"] = "Primary";
	genres["1.3.1.2"] = "Secondary";
	genres["1.3.1.3"] = "Tertiary";
	genres["1.3.2"] = "Lifelong/further education";
	genres["1.4"] = "Preview";
	genres["1.5"] = "Pause";
	genres["1.6"] = "FastForward";
	genres["1.6.1"] = "Gambling";
	genres["1.6.2"] = "Home Shopping";
	genres["1.7"] = "Rewind";
	genres["1.7.1"] = "Fund Raising";
	genres["1.7.2"] = "Social Action";
	genres["1.8"] = "SkipForward";
	genres["1.8.1"] = "General enrichment";
	genres["1.8.2"] = "Inspirational enrichment";
	genres["1.9"] = "SkipBackward";
	genres["1.9.1"] = "Very Easy";
	genres["1.9.2"] = "Easy";
	genres["1.9.3"] = "Medium";
	genres["1.9.4"] = "Difficult";
	genres["1.9.5"] = "Very Difficult";
	genres["10"] = "For more information";
	genres["11"] = "Programme review information";
	genres["12"] = "Recap";
	genres["13"] = "The making of";
	genres["14"] = "Support";
	genres["15"] = "Segmentation";
	genres["16"] = "Derived";
	genres["17"] = "TVA RMPI document";
	genres["18"] = "Content Package";
	genres["2"] = "Video ";
	genres["2.0"] = "Proprietary";
	genres["2.1"] = "Zoom";
	genres["2.1.1 "] = "Bulletin";
	genres["2.1.2"] = "Magazine";
	genres["2.1.2.1"] = "Presenter led magazine";
	genres["2.1.2.2"] = "Clip led magazine";
	genres["2.1.3"] = "Event";
	genres["2.1.3.1"] = "Commented event";
	genres["2.1.3.2"] = "Uncommented event";
	genres["2.1.4"] = "Documentary";
	genres["2.1.5"] = "Discussion/Interview/Debate/Talkshow";
	genres["2.1.6"] = "Lecture/Speech/Presentation";
	genres["2.1.7"] = "Textual (incl. relayed teletext)";
	genres["2.1.8"] = "Phone-in";
	genres["2.1.9"] = "DJ with discs";
	genres["2.2"] = "SlowMotion";
	genres["2.2.1"] = "Fictional portrayal of life";
	genres["2.2.2"] = "Readings";
	genres["2.2.3"] = "Dramatic documentary";
	genres["2.3"] = "CCOn";
	genres["2.3.1"] = "Anime";
	genres["2.3.2"] = "Computer";
	genres["2.3.3"] = "Cartoon";
	genres["2.3.4"] = "Puppetry";
	genres["2.3.4.1"] = "Real time puppetry";
	genres["2.3.4.2"] = "Physical model animation";
	genres["2.4"] = "StepForward";
	genres["2.4.1"] = "Hosted show";
	genres["2.4.1.1"] = "Simple game show";
	genres["2.4.1.2"] = "Big game show";
	genres["2.4.2"] = "Panel-show";
	genres["2.4.2.1"] = "Simple game show";
	genres["2.4.2.2"] = "Big game show";
	genres["2.4.3"] = "Non-hosted show";
	genres["2.4.4"] = "Standup comedian(s)";
	genres["2.4.5"] = "Reality Show";
	genres["2.4.5.1"] = "Observational show";
	genres["2.4.5.2"] = "Controlled show";
	genres["2.5"] = "StepBackward";
	genres["2.5.1"] = "Solo performance";
	genres["2.5.2"] = "Small ensemble performance";
	genres["2.5.3"] = "Large ensemble performance";
	genres["2.5.4"] = "Mixed";
	genres["2.6"] = "void";
	genres["2.7"] = "INTERACTIVE";
	genres["2.7.1"] = "LOCAL INTERACTIVITY";
	genres["2.7.1.1"] = "Static informational";
	genres["2.7.1.10"] = "Elimination and timer";
	genres["2.7.1.11"] = "Categories";
	genres["2.7.1.12"] = "Level based quiz/game";
	genres["2.7.1.13"] = "Following a sequence";
	genres["2.7.1.14"] = "Local multi player";
	genres["2.7.1.15"] = "Multi stream audio-video";
	genres["2.7.1.16"] = "Enhanced advertisement";
	genres["2.7.1.17"] = "Logic based games";
	genres["2.7.1.18"] = "Word games";
	genres["2.7.1.19"] = "Positional games";
	genres["2.7.1.2"] = "Dynamic informational";
	genres["2.7.1.20"] = "Board games";
	genres["2.7.1.21"] = "Text based gaming";
	genres["2.7.1.22"] = "Dynamic 2D/3D graphics";
	genres["2.7.1.3"] = "Viewing chats";
	genres["2.7.1.4"] = "Quiz - Basic multiple choice";
	genres["2.7.1.5"] = "Quiz - Text or number entry answers";
	genres["2.7.1.6"] = "Re-ordering";
	genres["2.7.1.7"] = "Positional";
	genres["2.7.1.8"] = "Sync quiz";
	genres["2.7.1.9"] = "Timer quiz";
	genres["2.7.2"] = "INTERMITTENT RESPONSE";
	genres["2.7.2.1"] = "Single impulse vote";
	genres["2.7.2.10"] = "Multi player TS networked services/games";
	genres["2.7.2.11"] = "Interactive advertisement";
	genres["2.7.2.2"] = "Impulse vote from choices";
	genres["2.7.2.3"] = "Impulse Yes/No vote";
	genres["2.7.2.4"] = "Impulse vote with a value";
	genres["2.7.2.5"] = "Submit answers/form";
	genres["2.7.2.6"] = "SMS using mobile";
	genres["2.7.2.7"] = "SMS using TV remote";
	genres["2.7.2.8"] = "Impulse gambling";
	genres["2.7.2.9"] = "Impulse transaction";
	genres["2.7.3"] = "ALWAYS ON CONNECTION";
	genres["2.7.3.1"] = "Chat Forum";
	genres["2.7.3.10"] = "Impulse transaction";
	genres["2.7.3.11"] = "Non-linear audio-video";
	genres["2.7.3.2"] = "Chat Forum via web";
	genres["2.7.3.3"] = "Threaded mail discussions";
	genres["2.7.3.4"] = "Point to point";
	genres["2.7.3.5"] = "3rd party point to point";
	genres["2.7.3.6"] = "Voice chat using mic capability";
	genres["2.7.3.7"] = "Dual player networked services/games";
	genres["2.7.3.8"] = "Multi player RT networked services/games";
	genres["2.7.3.9"] = "Gambling services";
	genres["3"] = "Data";
	genres["3.0"] = "Proprietary";
	genres["3.1"] = "ClickThrough";
	genres["3.1.1"] = "News";
	genres["3.1.1.1"] = "Daily news";
	genres["3.1.1.10"] = "Cultural";
	genres["3.1.1.10.1"] = "Arts";
	genres["3.1.1.10.2"] = "Entertainment";
	genres["3.1.1.10.3"] = "Film";
	genres["3.1.1.10.4"] = "Music";
	genres["3.1.1.10.5"] = "Radio";
	genres["3.1.1.10.6"] = "TV";
	genres["3.1.1.11"] = "Local/Regional";
	genres["3.1.1.12"] = "Traffic";
	genres["3.1.1.13"] = "Weather forecasts";
	genres["3.1.1.14"] = "Service information";
	genres["3.1.1.15"] = "Public affairs";
	genres["3.1.1.16"] = "Current affairs";
	genres["3.1.1.17"] = "Consumer affairs";
	genres["3.1.1.2"] = "Special news/edition";
	genres["3.1.1.3"] = "Special Report";
	genres["3.1.1.4"] = "Commentary";
	genres["3.1.1.5"] = "Periodical/General";
	genres["3.1.1.6"] = "National politics/National assembly";
	genres["3.1.1.7"] = "Economy/Market/Financial/Business";
	genres["3.1.1.8"] = "Foreign/International";
	genres["3.1.1.9"] = "Sports";
	genres["3.1.10"] = "Media";
	genres["3.1.10.1"] = "Advertising";
	genres["3.1.10.2"] = "Print media";
	genres["3.1.10.3"] = "Television";
	genres["3.1.10.4"] = "Radio";
	genres["3.1.10.5"] = "New media";
	genres["3.1.10.6"] = "Marketing";
	genres["3.1.11"] = "Listings";
	genres["3.1.2"] = "Religion/Philosophies";
	genres["3.1.2.1"] = "Religion";
	genres["3.1.2.1.1"] = "Buddhism";
	genres["3.1.2.1.10"] = "Confucianism";
	genres["3.1.2.1.11"] = "Jainism";
	genres["3.1.2.1.12"] = "Sikhism";
	genres["3.1.2.1.13"] = "Taoism";
	genres["3.1.2.1.14"] = "Vodun (Voodoo)";
	genres["3.1.2.1.15"] = "Asatru (Nordic Paganism)";
	genres["3.1.2.1.16"] = "Drudism";
	genres["3.1.2.1.17"] = "Goddess worship";
	genres["3.1.2.1.18"] = "Wicca";
	genres["3.1.2.1.19"] = "Witchcraft";
	genres["3.1.2.1.2"] = "Hinduism";
	genres["3.1.2.1.20"] = "Caodaism";
	genres["3.1.2.1.21"] = "Damanhur Community";
	genres["3.1.2.1.22"] = "Druse (Mowahhidoon)";
	genres["3.1.2.1.23"] = "Eckankar";
	genres["3.1.2.1.24"] = "Gnosticism";
	genres["3.1.2.1.25"] = "Rroma (Gypsies)";
	genres["3.1.2.1.26"] = "Hare Krishna and ISKCON";
	genres["3.1.2.1.27"] = "Lukumi (Santeria)";
	genres["3.1.2.1.28"] = "Macumba";
	genres["3.1.2.1.29"] = "Native American spirituality";
	genres["3.1.2.1.3"] = "Christianity";
	genres["3.1.2.1.30"] = "New Age";
	genres["3.1.2.1.31"] = "Osho";
	genres["3.1.2.1.32"] = "Satanism";
	genres["3.1.2.1.33"] = "Scientology";
	genres["3.1.2.1.34"] = "Thelema";
	genres["3.1.2.1.35"] = "Unitarian-Universalism";
	genres["3.1.2.1.36"] = "The Creativity Movement";
	genres["3.1.2.1.37"] = "Zoroastrianism";
	genres["3.1.2.1.38"] = "Quakerism";
	genres["3.1.2.1.39"] = "Rastafarianism";
	genres["3.1.2.1.4"] = "Islam";
	genres["3.1.2.1.5"] = "Judaism";
	genres["3.1.2.1.6"] = "Atheism";
	genres["3.1.2.1.7"] = "Agnosticism";
	genres["3.1.2.1.8"] = "Shintoism";
	genres["3.1.2.1.9"] = "Baha'i";
	genres["3.1.2.2"] = "Non-religious philosophies";
	genres["3.1.2.2.1"] = "Communism";
	genres["3.1.2.2.10"] = "Universism";
	genres["3.1.2.2.11"] = "Atheism";
	genres["3.1.2.2.12"] = "Agnosticism";
	genres["3.1.2.2.2"] = "Humanism";
	genres["3.1.2.2.3"] = "Capitalism";
	genres["3.1.2.2.4"] = "Socialism";
	genres["3.1.2.2.5"] = "Libertarianism";
	genres["3.1.2.2.6"] = "Republicanism";
	genres["3.1.2.2.7"] = "Deism";
	genres["3.1.2.2.8"] = "Falun Gong and Falun Dafa";
	genres["3.1.2.2.9"] = "Objectivism";
	genres["3.1.3"] = "General non-fiction";
	genres["3.1.3.1"] = "Political";
	genres["3.1.3.1.1"] = "Capitalism";
	genres["3.1.3.1.2"] = "Fascism";
	genres["3.1.3.1.3"] = "Republicanism";
	genres["3.1.3.1.4"] = "Socialism";
	genres["3.1.3.10"] = "Agriculture";
	genres["3.1.3.11"] = "Construction / Civil Engineering";
	genres["3.1.3.2"] = "Social";
	genres["3.1.3.3"] = "Economic";
	genres["3.1.3.4"] = "Legal";
	genres["3.1.3.5"] = "Finance";
	genres["3.1.3.6"] = "Education";
	genres["3.1.3.6.1"] = "Pre-School";
	genres["3.1.3.6.10"] = "Religious schools";
	genres["3.1.3.6.11"] = "Student organisations";
	genres["3.1.3.6.12"] = "Testing";
	genres["3.1.3.6.13"] = "Theory and methods";
	genres["3.1.3.6.14"] = "Interdisciplinary studies";
	genres["3.1.3.6.2"] = "Primary";
	genres["3.1.3.6.3"] = "Secondary";
	genres["3.1.3.6.4"] = "Colleges and Universities";
	genres["3.1.3.6.5"] = "Adult education";
	genres["3.1.3.6.6"] = "Non-formal education";
	genres["3.1.3.6.7"] = "Homework";
	genres["3.1.3.6.8"] = "Reading groups";
	genres["3.1.3.6.9"] = "Distance learning";
	genres["3.1.3.7"] = "International affairs";
	genres["3.1.3.8"] = "Military/Defence";
	genres["3.1.3.9"] = "Industry/Manufacturing";
	genres["3.1.4"] = "Arts";
	genres["3.1.4.1"] = "Music";
	genres["3.1.4.10"] = "Experimental arts";
	genres["3.1.4.11"] = "Architecture";
	genres["3.1.4.12"] = "Showbiz";
	genres["3.1.4.13"] = "Television";
	genres["3.1.4.14"] = "Radio";
	genres["3.1.4.15"] = "New media";
	genres["3.1.4.2"] = "Dance";
	genres["3.1.4.3"] = "Theatre";
	genres["3.1.4.4"] = "Opera";
	genres["3.1.4.5"] = "Cinema";
	genres["3.1.4.6"] = "Poetry";
	genres["3.1.4.7"] = "Press";
	genres["3.1.4.8"] = "Plastic arts";
	genres["3.1.4.9"] = "Fine arts";
	genres["3.1.5"] = "Humanities";
	genres["3.1.5.1"] = "Literature";
	genres["3.1.5.2"] = "Languages";
	genres["3.1.5.3"] = "History";
	genres["3.1.5.4"] = "Culture/Tradition/Anthropology/Ethnic studies";
	genres["3.1.5.5"] = "War/Conflict";
	genres["3.1.5.6"] = "Philosophy";
	genres["3.1.5.7"] = "Political Science";
	genres["3.1.6"] = "Sciences";
	genres["3.1.6.1"] = "Applied sciences";
	genres["3.1.6.10"] = "Psychology";
	genres["3.1.6.11"] = "Social";
	genres["3.1.6.12"] = "Spiritual";
	genres["3.1.6.13"] = "Mathematics";
	genres["3.1.6.14"] = "Archaeology";
	genres["3.1.6.15"] = "Statistics";
	genres["3.1.6.16"] = "Liberal Arts and Science";
	genres["3.1.6.2"] = "Nature/Natural sciences";
	genres["3.1.6.2.1"] = "Biology";
	genres["3.1.6.2.2"] = "Geology";
	genres["3.1.6.2.3"] = "Botany";
	genres["3.1.6.2.4"] = "Zoology";
	genres["3.1.6.3"] = "Animals/Wildlife";
	genres["3.1.6.4"] = "Environment/Geography";
	genres["3.1.6.5"] = "Space/Universe";
	genres["3.1.6.6"] = "Physical sciences";
	genres["3.1.6.6.1"] = "Physics";
	genres["3.1.6.6.2"] = "Chemistry";
	genres["3.1.6.6.3"] = "Mechanics";
	genres["3.1.6.6.4"] = "Engineering";
	genres["3.1.6.7"] = "Medicine";
	genres["3.1.6.7.1"] = "Alternative Medicine";
	genres["3.1.6.8"] = "Technology";
	genres["3.1.6.9"] = "Physiology";
	genres["3.1.7"] = "Human interest";
	genres["3.1.7.1"] = "Reality";
	genres["3.1.7.10"] = "libraries";
	genres["3.1.7.2"] = "Society/Show business/Gossip";
	genres["3.1.7.3"] = "Biography/Notable personalities";
	genres["3.1.7.4"] = "Personal problems";
	genres["3.1.7.5"] = "Investigative journalism";
	genres["3.1.7.6"] = "Museums";
	genres["3.1.7.7"] = "Religious buildings";
	genres["3.1.7.8"] = "Personal stories";
	genres["3.1.7.9"] = "Family life";
	genres["3.1.8"] = "Transport and Communications";
	genres["3.1.8.1"] = "Air";
	genres["3.1.8.2"] = "Land";
	genres["3.1.8.3"] = "Sea";
	genres["3.1.8.4"] = "Space";
	genres["3.1.9"] = "Events";
	genres["3.1.9.1"] = "Anniversary";
	genres["3.1.9.10"] = "Local/Regional";
	genres["3.1.9.11"] = "Seasonal";
	genres["3.1.9.12"] = "Sporting";
	genres["3.1.9.13"] = "Festival";
	genres["3.1.9.14"] = "Concert";
	genres["3.1.9.15"] = "Funeral/Memorial";
	genres["3.1.9.2"] = "Fair";
	genres["3.1.9.3"] = "Tradeshow";
	genres["3.1.9.4"] = "Musical";
	genres["3.1.9.5"] = "Exhibition";
	genres["3.1.9.6"] = "Royal";
	genres["3.1.9.7"] = "State";
	genres["3.1.9.8"] = "International";
	genres["3.1.9.9"] = "National";
	genres["3.10"] = "Archive";
	genres["3.2"] = "ScrollUp";
	genres["3.2.1"] = "Athletics";
	genres["3.2.1.1"] = "Field";
	genres["3.2.1.2"] = "Track";
	genres["3.2.1.3"] = "Combined athletics";
	genres["3.2.1.4"] = "Running";
	genres["3.2.1.5"] = "Cross-country";
	genres["3.2.1.6"] = "Triathlon";
	genres["3.2.10"] = "Gymnastics";
	genres["3.2.10.1"] = "Asymmetric bars";
	genres["3.2.10.2"] = "Beam";
	genres["3.2.10.3"] = "Horse";
	genres["3.2.10.4"] = "Mat";
	genres["3.2.10.5"] = "Parallel bars";
	genres["3.2.10.6"] = "Rings";
	genres["3.2.10.7"] = "Trampoline";
	genres["3.2.11"] = "Equestrian";
	genres["3.2.11.1"] = "Cart";
	genres["3.2.11.2"] = "Dressage";
	genres["3.2.11.3"] = "Horse racing";
	genres["3.2.11.4"] = "Polo";
	genres["3.2.11.5"] = "Jumping";
	genres["3.2.11.6"] = "Crossing";
	genres["3.2.11.7"] = "Trotting";
	genres["3.2.12"] = "Adventure sports";
	genres["3.2.12.1"] = "Archery";
	genres["3.2.12.10"] = "Skateboarding";
	genres["3.2.12.11"] = "Treking";
	genres["3.2.12.2"] = "Extreme sports";
	genres["3.2.12.3"] = "Mountaineering";
	genres["3.2.12.4"] = "Climbing";
	genres["3.2.12.5"] = "Orienteering";
	genres["3.2.12.6"] = "Shooting";
	genres["3.2.12.7"] = "Sport acrobatics";
	genres["3.2.12.8"] = "Rafting";
	genres["3.2.12.9"] = "Caving";
	genres["3.2.13"] = "Strength-based sports";
	genres["3.2.13.1"] = "Body-building";
	genres["3.2.13.2"] = "Boxing";
	genres["3.2.13.3"] = "Combative sports";
	genres["3.2.13.4"] = "Power-lifting";
	genres["3.2.13.5"] = "Weight-lifting";
	genres["3.2.13.6"] = "Wrestling";
	genres["3.2.14"] = "Air sports";
	genres["3.2.14.1"] = "Ballooning";
	genres["3.2.14.10"] = "Aerobatics";
	genres["3.2.14.2"] = "Hang gliding";
	genres["3.2.14.3"] = "Sky diving";
	genres["3.2.14.4"] = "Delta-plane";
	genres["3.2.14.5"] = "Parachuting";
	genres["3.2.14.6"] = "Kiting";
	genres["3.2.14.7"] = "Aeronautics";
	genres["3.2.14.8"] = "Gliding";
	genres["3.2.14.9"] = "Flying";
	genres["3.2.15"] = "Golf";
	genres["3.2.16"] = "Fencing";
	genres["3.2.17"] = "Dog racing";
	genres["3.2.18"] = "Casting";
	genres["3.2.19"] = "Maccabi";
	genres["3.2.2"] = "Cycling/Bicycle";
	genres["3.2.2.1"] = "Mountainbike";
	genres["3.2.2.2"] = "Bicross";
	genres["3.2.2.3"] = "Indoor cycling";
	genres["3.2.2.4"] = "Road cycling";
	genres["3.2.20"] = "Modern Pentathlon";
	genres["3.2.21"] = "Sombo";
	genres["3.2.22"] = "Mind Games";
	genres["3.2.22.1"] = "Bridge";
	genres["3.2.22.2"] = "Chess";
	genres["3.2.22.3"] = "Poker";
	genres["3.2.23"] = "Traditional Games";
	genres["3.2.24"] = "Disabled Sport";
	genres["3.2.24.1"] = "Physically Challenged";
	genres["3.2.24.2"] = "Mentally Challenged";
	genres["3.2.3"] = "Team sports";
	genres["3.2.3.1"] = "Football (American)";
	genres["3.2.3.10"] = "Croquet";
	genres["3.2.3.11"] = "Faustball";
	genres["3.2.3.12"] = "Football (Soccer)";
	genres["3.2.3.13"] = "Handball";
	genres["3.2.3.14"] = "Hockey";
	genres["3.2.3.15"] = "Korfball";
	genres["3.2.3.16"] = "Lacrosse";
	genres["3.2.3.17"] = "Netball";
	genres["3.2.3.18"] = "Roller skating";
	genres["3.2.3.19"] = "Rugby";
	genres["3.2.3.19.1"] = "Rugby union";
	genres["3.2.3.19.2"] = "Rugby league";
	genres["3.2.3.2"] = "Football (Australian)";
	genres["3.2.3.20"] = "Softball";
	genres["3.2.3.21"] = "Volleyball";
	genres["3.2.3.22"] = "Beach volley";
	genres["3.2.3.23"] = "Hurling";
	genres["3.2.3.24"] = "Flying Disc/ Frisbee";
	genres["3.2.3.25"] = "Kabadi";
	genres["3.2.3.26"] = "Camogie";
	genres["3.2.3.27"] = "Shinty";
	genres["3.2.3.28"] = "Street Soccer";
	genres["3.2.3.3"] = "Football (Gaelic)";
	genres["3.2.3.4"] = "Football (Indoor)";
	genres["3.2.3.5"] = "Beach soccer";
	genres["3.2.3.6"] = "Bandy";
	genres["3.2.3.7"] = "Baseball";
	genres["3.2.3.8"] = "Basketball";
	genres["3.2.3.9"] = "Cricket";
	genres["3.2.4"] = "Racket sports";
	genres["3.2.4.1"] = "Badminton";
	genres["3.2.4.2"] = "Racketball";
	genres["3.2.4.3"] = "Short tennis";
	genres["3.2.4.4"] = "Soft tennis";
	genres["3.2.4.5"] = "Squash";
	genres["3.2.4.6"] = "Table tennis";
	genres["3.2.4.7"] = "Tennis";
	genres["3.2.5"] = "Martial Arts";
	genres["3.2.5.1"] = "Aikido";
	genres["3.2.5.2"] = "Jai-alai";
	genres["3.2.5.3"] = "Judo";
	genres["3.2.5.4"] = "Ju-jitsu";
	genres["3.2.5.5"] = "Karate";
	genres["3.2.5.6"] = "Sumo/Fighting games";
	genres["3.2.5.7"] = "Sambo";
	genres["3.2.5.8"] = "Taekwondo";
	genres["3.2.6"] = "Water sports";
	genres["3.2.6.1"] = "Bodyboarding";
	genres["3.2.6.10"] = "Surfing";
	genres["3.2.6.11"] = "Swimming";
	genres["3.2.6.12"] = "Water polo";
	genres["3.2.6.13"] = "Water skiing";
	genres["3.2.6.14"] = "Windsurfing";
	genres["3.2.6.2"] = "Yatching";
	genres["3.2.6.3"] = "Canoeing";
	genres["3.2.6.4"] = "Diving";
	genres["3.2.6.5"] = "Fishing";
	genres["3.2.6.6"] = "Polo";
	genres["3.2.6.7"] = "Rowing";
	genres["3.2.6.8"] = "Sailing";
	genres["3.2.6.9"] = "Sub-aquatics";
	genres["3.2.7"] = "Winter sports";
	genres["3.2.7.1"] = "Bobsleigh/Tobogganing";
	genres["3.2.7.10"] = "Snowboarding";
	genres["3.2.7.11"] = "Alpine skiing";
	genres["3.2.7.12"] = "Freestyle skiing ";
	genres["3.2.7.13"] = "Inline skating";
	genres["3.2.7.14"] = "Nordic skiing";
	genres["3.2.7.15"] = "Ski jumping";
	genres["3.2.7.16"] = "Speed skating";
	genres["3.2.7.17"] = "Figure skating";
	genres["3.2.7.18"] = "Ice-dance";
	genres["3.2.7.19"] = "Marathon";
	genres["3.2.7.2"] = "Curling";
	genres["3.2.7.20"] = "Short-track";
	genres["3.2.7.21"] = "Biathlon";
	genres["3.2.7.3"] = "Ice-hockey";
	genres["3.2.7.4"] = "Ice-skating";
	genres["3.2.7.5"] = "Luge";
	genres["3.2.7.6"] = "Skating";
	genres["3.2.7.7"] = "Skibob";
	genres["3.2.7.8"] = "Skiing";
	genres["3.2.7.9"] = "Sleddog";
	genres["3.2.8"] = "Motor sports";
	genres["3.2.8.1"] = "Auto racing";
	genres["3.2.8.10"] = "Stock car";
	genres["3.2.8.11"] = "Hill Climb";
	genres["3.2.8.12"] = "Trials";
	genres["3.2.8.2"] = "Motor boating";
	genres["3.2.8.3"] = "Motor cycling";
	genres["3.2.8.4"] = "Formula 1";
	genres["3.2.8.5"] = "Indy car";
	genres["3.2.8.6"] = "Karting";
	genres["3.2.8.7"] = "Rally";
	genres["3.2.8.8"] = "Trucking";
	genres["3.2.8.9"] = "Tractor pulling";
	genres["3.2.9"] = "'Social' sports";
	genres["3.2.9.1"] = "Billiards";
	genres["3.2.9.10"] = "Balle pelote";
	genres["3.2.9.11"] = "Basque pelote";
	genres["3.2.9.12"] = "Trickshot";
	genres["3.2.9.2"] = "Boules";
	genres["3.2.9.3"] = "Bowling";
	genres["3.2.9.4"] = "Chess";
	genres["3.2.9.5"] = "Dance sport";
	genres["3.2.9.6"] = "Darts";
	genres["3.2.9.7"] = "Pool";
	genres["3.2.9.8"] = "Snooker";
	genres["3.2.9.9"] = "Tug-of-war";
	genres["3.3"] = "ScrollDown";
	genres["3.3.1"] = "Do-it-yourself";
	genres["3.3.10"] = "Art";
	genres["3.3.11"] = "Music";
	genres["3.3.12"] = "Board Games";
	genres["3.3.13"] = "Computer Games";
	genres["3.3.14"] = "Card Cames";
	genres["3.3.15"] = "Fitness/Keep-fit";
	genres["3.3.16"] = "Personal health";
	genres["3.3.17"] = "Car";
	genres["3.3.18"] = "Motorcycle/Motoring";
	genres["3.3.19"] = "Fashion";
	genres["3.3.2"] = "Cookery";
	genres["3.3.20"] = "Life/ House Keeping/Lifestyle";
	genres["3.3.21"] = "Technology/Computing";
	genres["3.3.22"] = "Gaming";
	genres["3.3.23"] = "Shopping";
	genres["3.3.24"] = "Adult";
	genres["3.3.25"] = "Road safety";
	genres["3.3.26"] = "Consumer advice";
	genres["3.3.27"] = "Employment advice";
	genres["3.3.28"] = "Boating";
	genres["3.3.29"] = "Parenting";
	genres["3.3.3"] = "Gardening";
	genres["3.3.30"] = "Self-help";
	genres["3.3.31"] = "Collectibles";
	genres["3.3.32"] = "Jewellery";
	genres["3.3.33"] = "Beauty";
	genres["3.3.34"] = "Aviation";
	genres["3.3.4"] = "Travel/Tourism";
	genres["3.3.5"] = "Adventure/Expeditions";
	genres["3.3.6"] = "Fishing";
	genres["3.3.7"] = "Outdoor";
	genres["3.3.8"] = "Pet";
	genres["3.3.9"] = "Craft/Handicraft";
	genres["3.4"] = "ViewGuide";
	genres["3.4.1"] = "General light drama";
	genres["3.4.10"] = "Musical";
	genres["3.4.11"] = "Comedy";
	genres["3.4.12"] = "Effect movies";
	genres["3.4.13"] = "Classical drama";
	genres["3.4.14"] = "Period drama";
	genres["3.4.15"] = "Contemporary drama";
	genres["3.4.16"] = "Religious";
	genres["3.4.17"] = "Poems/Stories";
	genres["3.4.18"] = "Biography";
	genres["3.4.19"] = "Psychological drama";
	genres["3.4.2"] = "Soap";
	genres["3.4.2.1"] = "Soap opera";
	genres["3.4.2.2"] = "Soap special";
	genres["3.4.2.3"] = "Soap talk";
	genres["3.4.3"] = "Romance";
	genres["3.4.4"] = "Legal Melodrama";
	genres["3.4.5"] = "Medical melodrama";
	genres["3.4.6"] = "Action";
	genres["3.4.6.1"] = "Adventure";
	genres["3.4.6.10"] = "Thriller";
	genres["3.4.6.11"] = "Sports";
	genres["3.4.6.12"] = "Martial arts";
	genres["3.4.6.13"] = "Epic";
	genres["3.4.6.2"] = "Disaster";
	genres["3.4.6.3"] = "Mystery";
	genres["3.4.6.4"] = "Detective/Police";
	genres["3.4.6.5"] = "Historical/Epic";
	genres["3.4.6.6"] = "Horror";
	genres["3.4.6.7"] = "Science fiction";
	genres["3.4.6.8"] = "War";
	genres["3.4.6.9"] = "Western";
	genres["3.4.7"] = "Fantasy/Fairy tale";
	genres["3.4.8"] = "Erotica";
	genres["3.4.9"] = "Drama based on real events (docudrama)";
	genres["3.5"] = "SavePage";
	genres["3.5.1"] = "Game show";
	genres["3.5.10"] = "Magic/Hypnotism";
	genres["3.5.11"] = "Circus";
	genres["3.5.12"] = "Dating";
	genres["3.5.13"] = "Bullfighting";
	genres["3.5.14"] = "Rodeo";
	genres["3.5.15"] = "Airshow";
	genres["3.5.16"] = "Chat";
	genres["3.5.2"] = "Quiz/Contest";
	genres["3.5.2.1"] = "Quiz";
	genres["3.5.2.2"] = "Contest";
	genres["3.5.3"] = "Variety/Talent";
	genres["3.5.3.1"] = "Cabaret";
	genres["3.5.3.2"] = "Talent";
	genres["3.5.4"] = "Surprise";
	genres["3.5.5"] = "Reality";
	genres["3.5.6"] = "Candid camera";
	genres["3.5.7"] = "Comedy";
	genres["3.5.7.1"] = "Broken comedy";
	genres["3.5.7.2"] = "Romantic comedy";
	genres["3.5.7.3"] = "Sitcom";
	genres["3.5.7.4"] = "Satire";
	genres["3.5.7.5"] = "Candid Camera";
	genres["3.5.7.6"] = "Humour";
	genres["3.5.9"] = "Humour";
	genres["3.50"] = "Commercial/Products";
	genres["3.50.1"] = "Agriculture, forestry and fishery products";
	genres["3.50.1.1"] = "Products of agriculture, horticulture and market gardening";
	genres["3.50.1.2"] = "Live animals and animal products";
	genres["3.50.1.3"] = "Forestry and logging products";
	genres["3.50.1.4"] = "Fish and other fishing products";
	genres["3.50.10"] = "Community, social and personal services";
	genres["3.50.10.1"] = "Public administration and other services to the community as a whole; compulsory social security services";
	genres["3.50.10.2"] = "Education services";
	genres["3.50.10.3"] = "Health and social services";
	genres["3.50.10.4"] = "Sewage and refuse disposal, sanitation and other environmental protection services";
	genres["3.50.10.5"] = "Services of membership organizations";
	genres["3.50.10.6"] = "Recreational, cultural and sporting services";
	genres["3.50.10.7"] = "Other services";
	genres["3.50.10.8"] = "Domestic services";
	genres["3.50.10.9"] = "Services provided by extraterritorial organizations and bodies";
	genres["3.50.2"] = "Ores and minerals; electricity, gas and water";
	genres["3.50.2.1"] = "Coal and lignite; peat";
	genres["3.50.2.2"] = "Crude petroleum and natural gas";
	genres["3.50.2.3"] = "Uranium and thorium ores";
	genres["3.50.2.4"] = "Metal ores";
	genres["3.50.2.5"] = "Stone, sand and clay";
	genres["3.50.2.6"] = "Other minerals";
	genres["3.50.2.7"] = "Electricity, town gas, steam and hot water";
	genres["3.50.2.8"] = "Water";
	genres["3.50.3"] = "Food products, beverages and tobacco; textiles, apparel and leather products";
	genres["3.50.3.1"] = "Meat, fish, fruit, vegetables, oils and fats";
	genres["3.50.3.2"] = "Dairy products";
	genres["3.50.3.3"] = "Grain mill products, starches and starch products; other food products";
	genres["3.50.3.4"] = "Beverages";
	genres["3.50.3.5"] = "Tobacco products";
	genres["3.50.3.6"] = "Yarn and thread; woven and tufted textile fabrics";
	genres["3.50.3.7"] = "Textile articles other than apparel";
	genres["3.50.3.8"] = "Knitted or crocheted fabrics; wearing apparel";
	genres["3.50.3.9"] = "Leather and leather products; footwear";
	genres["3.50.4"] = "Other transportable goods, except metal products, machinery and equipment";
	genres["3.50.4.1"] = "Products of wood, cork, straw and plaiting materials";
	genres["3.50.4.2"] = "Pulp, paper and paper products; printed matter and related articles";
	genres["3.50.4.3"] = "Coke oven products; refined petroleum products; nuclear fuel";
	genres["3.50.4.4"] = "Basic chemicals";
	genres["3.50.4.5"] = "Other chemical products; man-made fibres";
	genres["3.50.4.6"] = "Rubber and plastics products";
	genres["3.50.4.7"] = "Glass and glass products and other non-metallic products n.e.c.";
	genres["3.50.4.8"] = "Furniture; other transportable goods n.e.c.";
	genres["3.50.4.9"] = "Wastes or scraps";
	genres["3.50.5"] = "Metal products, machinery and equipment";
	genres["3.50.5.1"] = "Basic metals";
	genres["3.50.5.2"] = "Fabricated metal products, except machinery and equipment";
	genres["3.50.5.3"] = "General purpose machinery";
	genres["3.50.5.4"] = "Special purpose machinery";
	genres["3.50.5.5"] = "Office, accounting and computing machinery";
	genres["3.50.5.6"] = "Electrical machinery and apparatus";
	genres["3.50.5.7"] = "Radio, television and communication equipment and apparatus";
	genres["3.50.5.8"] = "Medical appliances, precision and optical instruments, watches and clocks";
	genres["3.50.5.9"] = "Transport equipment";
	genres["3.50.6"] = "Intangible assets; land; constructions; construction services";
	genres["3.50.6.1"] = "Intangible assets";
	genres["3.50.6.2"] = "Land";
	genres["3.50.6.3"] = "Constructions";
	genres["3.50.6.4"] = "Construction services";
	genres["3.50.7"] = "Distributive trade services; lodging; food and beverage serving services; transport services; and utilities distribution services";
	genres["3.50.7.1"] = "Wholesale trade services";
	genres["3.50.7.2"] = "Retail trade services";
	genres["3.50.7.3"] = "Lodging; food and beverage serving services";
	genres["3.50.7.4"] = "Land transport services";
	genres["3.50.7.5"] = "Water transport services";
	genres["3.50.7.6"] = "Air transport services";
	genres["3.50.7.7"] = "Supporting and auxiliary transport services";
	genres["3.50.7.8"] = "Postal and courier services";
	genres["3.50.7.9"] = "Electricity distribution services; gas and water distribution services through mains";
	genres["3.50.8"] = "Financial and related services; real estate services; and rental and leasing services";
	genres["3.50.8.1"] = "Financial intermediation, insurance and auxiliary services";
	genres["3.50.8.2"] = "Real estate services";
	genres["3.50.8.3"] = "Leasing or rental services without operator";
	genres["3.50.9"] = "Business and production services";
	genres["3.50.9.1"] = "Research and development services";
	genres["3.50.9.2"] = "Professional, scientific and technical services";
	genres["3.50.9.3"] = "Other professional, scientific and technical services";
	genres["3.50.9.4"] = "Telecommunications services; information retrieval and supply services";
	genres["3.50.9.5"] = "Support services";
	genres["3.50.9.6"] = "Production services, on a fee or contract basis";
	genres["3.50.9.7"] = "Maintenance and repair services";
	genres["3.6"] = "PrintPage";
	genres["3.6..5.2.1"] = "Hip Hop Soul";
	genres["3.6..5.2.2"] = "Neo Soul";
	genres["3.6..5.2.3"] = "New Jack Swing";
	genres["3.6.1"] = "Classical music";
	genres["3.6.1.1"] = "Early";
	genres["3.6.1.10"] = "Solo instruments";
	genres["3.6.1.11"] = "Chamber";
	genres["3.6.1.12"] = "Symphonic";
	genres["3.6.1.13"] = "Vocal";
	genres["3.6.1.14"] = "Choral";
	genres["3.6.1.15"] = "Song";
	genres["3.6.1.16"] = "Orchestral";
	genres["3.6.1.17"] = "Organ";
	genres["3.6.1.18"] = "String Quartet";
	genres["3.6.1.19"] = "Experimental/Avant Garde";
	genres["3.6.1.2"] = "Classical";
	genres["3.6.1.3"] = "Romantic";
	genres["3.6.1.4"] = "Contemporary";
	genres["3.6.1.5"] = "Light classical";
	genres["3.6.1.6"] = "Middle Ages";
	genres["3.6.1.7"] = "Renaissance";
	genres["3.6.1.8"] = "Baroque";
	genres["3.6.1.9"] = "Opera";
	genres["3.6.10"] = "Hit-Chart/Song Requests";
	genres["3.6.11"] = "Children's songs";
	genres["3.6.12"] = "Event music";
	genres["3.6.12.1"] = "Wedding";
	genres["3.6.12.2"] = "Sports";
	genres["3.6.12.3"] = "Ceremonial/Chants";
	genres["3.6.13"] = "Spoken";
	genres["3.6.14"] = "Dance";
	genres["3.6.14.1"] = "Ballet";
	genres["3.6.14.2"] = "Tap";
	genres["3.6.14.3"] = "Modern";
	genres["3.6.14.4"] = "Classical";
	genres["3.6.14.5"] = "Ballroom";
	genres["3.6.15"] = "Religious music";
	genres["3.6.16"] = "Era";
	genres["3.6.16.1"] = "Medieval (before 1400)";
	genres["3.6.16.2"] = "Renaissance (1400-1600)";
	genres["3.6.16.3"] = "Baroque (1600-1760)";
	genres["3.6.16.4"] = "Classical (1730-1820)";
	genres["3.6.16.5"] = "Romantic (1815-1910";
	genres["3.6.16.6"] = "20th Century";
	genres["3.6.16.6.1"] = "1910s";
	genres["3.6.16.6.2"] = "1920s";
	genres["3.6.16.6.3"] = "1930s";
	genres["3.6.16.6.4"] = "1940s";
	genres["3.6.16.6.5"] = "1950s";
	genres["3.6.16.6.6"] = "1960s";
	genres["3.6.16.6.7"] = "1970s";
	genres["3.6.16.6.8"] = "1980s";
	genres["3.6.16.6.9"] = "1990s";
	genres["3.6.16.7"] = "21st Century";
	genres["3.6.16.7.1"] = "2000s";
	genres["3.6.16.7.10"] = "2090s";
	genres["3.6.16.7.2"] = "2010s";
	genres["3.6.16.7.3"] = "2020s";
	genres["3.6.16.7.4"] = "2030s";
	genres["3.6.16.7.5"] = "2040s";
	genres["3.6.16.7.6"] = "2050s";
	genres["3.6.16.7.7"] = "2060s";
	genres["3.6.16.7.8"] = "2070s";
	genres["3.6.16.7.9"] = "2080s";
	genres["3.6.2"] = "Jazz";
	genres["3.6.2.1"] = "New Orleans/Early jazz";
	genres["3.6.2.10"] = "Acid jazz/Fusion";
	genres["3.6.2.2"] = "Big band/Swing/Dixie";
	genres["3.6.2.3"] = "Blues/Soul jazz";
	genres["3.6.2.4"] = "Bop/Hard bop/Bebop/Postbop";
	genres["3.6.2.5"] = "Traditional/Smooth";
	genres["3.6.2.6"] = "Cool";
	genres["3.6.2.7"] = "Modern/Avant-garde/Free";
	genres["3.6.2.8"] = "Latin and World jazz";
	genres["3.6.2.9"] = "Pop jazz/Jazz funk";
	genres["3.6.3"] = "Background music";
	genres["3.6.3.1"] = "Middle-of-the-road";
	genres["3.6.3.10"] = "Showtunes";
	genres["3.6.3.11"] = "TV";
	genres["3.6.3.12"] = "Cabaret";
	genres["3.6.3.13"] = "Instrumental";
	genres["3.6.3.14"] = "Sound clip";
	genres["3.6.3.15"] = "Retro";
	genres["3.6.3.2"] = "Easy listening";
	genres["3.6.3.3"] = "Ambient";
	genres["3.6.3.4"] = "Mood music";
	genres["3.6.3.5"] = "Oldies";
	genres["3.6.3.6"] = "Love songs";
	genres["3.6.3.7"] = "Dance hall";
	genres["3.6.3.8"] = "Soundtrack";
	genres["3.6.3.9"] = "Trailer";
	genres["3.6.4"] = "Pop-rock";
	genres["3.6.4.1"] = "Pop";
	genres["3.6.4.10"] = "Progressive/Alternative/Indie/Experimental/Art-rock";
	genres["3.6.4.11"] = "Seasonal/Holiday";
	genres["3.6.4.12"] = "Japanese pop-rock";
	genres["3.6.4.13"] = "Karaoke/Singing contests";
	genres["3.6.4.14"] = "Rock";
	genres["3.6.4.14.1"] = "AOR / Slow Rock / Soft Rock";
	genres["3.6.4.14.10"] = "Nu Punk";
	genres["3.6.4.14.11"] = "Grunge";
	genres["3.6.4.14.12"] = "Garage Punk/Psychedelia";
	genres["3.6.4.14.13"] = "Heavy Rock";
	genres["3.6.4.14.2"] = "Metal";
	genres["3.6.4.14.3"] = "Glam Rock";
	genres["3.6.4.14.4"] = "Punk Rock";
	genres["3.6.4.14.5"] = "Prog / Symphonic Rock";
	genres["3.6.4.14.6"] = "Alternative / Indie";
	genres["3.6.4.14.7"] = "Experimental / Avant Garde";
	genres["3.6.4.14.8"] = "Art Rock";
	genres["3.6.4.14.9"] = "Folk Rock";
	genres["3.6.4.15"] = "New Wave";
	genres["3.6.4.16"] = "Easy listening / Exotica";
	genres["3.6.4.17"] = "Singer/Songwriter";
	genres["3.6.4.2"] = "Chanson/Ballad";
	genres["3.6.4.3"] = "Traditional rock and roll";
	genres["3.6.4.4"] = "Soft/Slow rock";
	genres["3.6.4.5"] = "Classic/Dance/Pop-rock";
	genres["3.6.4.6"] = "Folk";
	genres["3.6.4.7"] = "Punk/Funk rock";
	genres["3.6.4.8"] = "New Age";
	genres["3.6.4.9"] = "Instrumental/Band/Symphonic rock/Jam bands";
	genres["3.6.5"] = "Blues/Rhythm and Blues/Soul/Gospel";
	genres["3.6.5.1"] = "Blues";
	genres["3.6.5.2"] = "R and B";
	genres["3.6.5.3"] = "Soul";
	genres["3.6.5.4"] = "Gospel";
	genres["3.6.5.5"] = "Rhythm and Blues";
	genres["3.6.5.6"] = "Funk";
	genres["3.6.5.6.1"] = "Afro Funk";
	genres["3.6.5.6.2"] = "Rare Groove";
	genres["3.6.6"] = "Country and Western";
	genres["3.6.7"] = "Rap/Hip Hop/Reggae";
	genres["3.6.7.1"] = "Rap/Christian rap";
	genres["3.6.7.1.1"] = "Gangsta Rap";
	genres["3.6.7.2"] = "Hip Hop/Trip-Hop";
	genres["3.6.7.2.1"] = "Dirty South Hip Hop";
	genres["3.6.7.2.2"] = "East Coast Hip Hop";
	genres["3.6.7.2.4"] = "UK Hip Hop";
	genres["3.6.7.2.5"] = "West Coast Hip Hop";
	genres["3.6.7.3"] = "Reggae";
	genres["3.6.7.3.1"] = "Dancehall";
	genres["3.6.7.3.2"] = "Dub";
	genres["3.6.7.3.3"] = "Lovers Rock";
	genres["3.6.7.3.4"] = "Raggamuffin";
	genres["3.6.7.3.5"] = "Rocksteady";
	genres["3.6.7.3.6"] = "Ska";
	genres["3.6.7.4"] = "Ska/Gangsta";
	genres["3.6.8"] = "Electronic/Club/Urban/Dance";
	genres["3.6.8.1"] = "Acid/Punk/Acid Punk";
	genres["3.6.8.10"] = "Metal/Death metal/Pop metal";
	genres["3.6.8.11"] = "Drum and Bass";
	genres["3.6.8.12"] = "Pranks";
	genres["3.6.8.13"] = "Grunge";
	genres["3.6.8.14"] = "Dance/Dance-pop";
	genres["3.6.8.15"] = "Garage (1990s)";
	genres["3.6.8.16"] = "UK Garage";
	genres["3.6.8.16.1"] = "2 Step";
	genres["3.6.8.16.2"] = "4/4 Vocal Garage";
	genres["3.6.8.16.3"] = "8 Bar";
	genres["3.6.8.16.4"] = "Dubstep";
	genres["3.6.8.16.5"] = "Eski-Beat";
	genres["3.6.8.16.6"] = "Grime";
	genres["3.6.8.16.7"] = "Soulful House and Garage";
	genres["3.6.8.16.8"] = "Speed Garage";
	genres["3.6.8.16.9"] = "Sublow";
	genres["3.6.8.17"] = "Breakbeat";
	genres["3.6.8.18"] = "Broken Beat";
	genres["3.6.8.2"] = "Disco";
	genres["3.6.8.22"] = "Ambient Dance";
	genres["3.6.8.23"] = "Alternative Dance";
	genres["3.6.8.3"] = "Techno/Euro-Techno/Techno-Industrial/Industrial";
	genres["3.6.8.4"] = "House/Techno House";
	genres["3.6.8.4.1"] = "Progressive House";
	genres["3.6.8.5"] = "Rave";
	genres["3.6.8.6"] = "Jungle/Tribal";
	genres["3.6.8.7"] = "Trance";
	genres["3.6.8.8"] = "Punk";
	genres["3.6.8.9"] = "Garage/Psychadelic";
	genres["3.6.9"] = "World/Traditional/Ethnic/Folk music";
	genres["3.6.9.1"] = "Africa";
	genres["3.6.9.10"] = "Modern";
	genres["3.6.9.2"] = "Asia";
	genres["3.6.9.3"] = "Australia/Oceania";
	genres["3.6.9.4"] = "Caribbean";
	genres["3.6.9.4.1"] = "Calypso";
	genres["3.6.9.4.2"] = "SOCA";
	genres["3.6.9.5"] = "Europe";
	genres["3.6.9.6"] = "Latin America";
	genres["3.6.9.7"] = "Middle East";
	genres["3.6.9.8"] = "North America";
	genres["3.6.9.9"] = "Fusion";
	genres["3.7"] = "Search";
	genres["3.7.1"] = "CONTENT GAMES CATEGORIES";
	genres["3.7.1.1"] = "Action";
	genres["3.7.1.10"] = "Sports";
	genres["3.7.1.11"] = "Strategy";
	genres["3.7.1.12"] = "Wrestling";
	genres["3.7.1.13"] = "Classic/Retro";
	genres["3.7.1.2"] = "Adventure";
	genres["3.7.1.3"] = "Fighting";
	genres["3.7.1.4"] = "Online";
	genres["3.7.1.5"] = "Platform";
	genres["3.7.1.6"] = "Puzzle";
	genres["3.7.1.7"] = "RPG/ MUDs";
	genres["3.7.1.8"] = "Racing";
	genres["3.7.1.9"] = "Simulation";
	genres["3.7.2"] = "STYLE";
	genres["3.7.2.1"] = "Logic based";
	genres["3.7.2.2"] = "Word games";
	genres["3.7.2.3"] = "Positional";
	genres["3.7.2.4"] = "Board games";
	genres["3.7.2.5"] = "Text environments";
	genres["3.7.2.6"] = "Dynamic 2D/3D graphics";
	genres["3.7.2.7"] = "Non-linear video";
	genres["3.8"] = "SubmitForm";
	genres["3.8.1"] = "General Consumer Advice";
	genres["3.8.1.1"] = "Road safety";
	genres["3.8.1.2"] = "Consumer advice";
	genres["3.8.1.3"] = "Employment Advice";
	genres["3.8.1.4"] = "Self-help";
	genres["3.8.2"] = "Computing/Technology";
	genres["3.8.2.1"] = "Technology/Computing";
	genres["3.8.2.2"] = "Computer Games";
	genres["3.8.3"] = "Cookery, Food, Drink";
	genres["3.8.3.1"] = "Cookery";
	genres["3.8.3.2"] = "Food and Drink";
	genres["3.8.4"] = "Homes/Interior/Gardening";
	genres["3.8.4.1"] = "Do-it-yourself";
	genres["3.8.4.2"] = "Home Improvement";
	genres["3.8.4.3"] = "Gardening";
	genres["3.8.4.4"] = "Property Buying and Selling";
	genres["3.8.5"] = "Hobbies";
	genres["3.8.5.1"] = "Fishing";
	genres["3.8.5.10"] = "Collectibles/Antiques";
	genres["3.8.5.11"] = "Jewellery";
	genres["3.8.5.12"] = "Aviation";
	genres["3.8.5.13"] = "Trains";
	genres["3.8.5.14"] = "Boating";
	genres["3.8.5.15"] = "Ornithology";
	genres["3.8.5.2"] = "Pet";
	genres["3.8.5.3"] = "Craft/Handicraft";
	genres["3.8.5.4"] = "Art";
	genres["3.8.5.5"] = "Music";
	genres["3.8.5.6"] = "Board Games";
	genres["3.8.5.7"] = "Card Cames";
	genres["3.8.5.8"] = "Gaming";
	genres["3.8.5.9"] = "Shopping";
	genres["3.8.6"] = "Cars and Motoring";
	genres["3.8.6.1"] = "Car";
	genres["3.8.6.2"] = "Motorcycle";
	genres["3.8.7"] = "Personal/Lifestyle/Family";
	genres["3.8.7.1"] = "Fitness / Keep-fit";
	genres["3.8.7.2"] = "Personal health";
	genres["3.8.7.3"] = "Fashion";
	genres["3.8.7.4"] = "House Keeping";
	genres["3.8.7.5"] = "Parenting";
	genres["3.8.7.6"] = "Beauty";
	genres["3.8.9"] = "Travel/Tourism";
	genres["3.8.9.1"] = "Holidays";
	genres["3.8.9.2"] = "Adventure/Expeditions";
	genres["3.8.9.3"] = "Outdoor pursuits";
	genres["3.9"] = "SubmitQuery";
	genres["4"] = "Commerce";
	genres["4.0"] = "Proprietary";
	genres["4.1"] = "Buy";
	genres["4.11"] = "LANGUAGE OF TARGET AUDIENCE";
	genres["4.2"] = "AddToWishList";
	genres["4.2.1"] = "Children";
	genres["4.2.1.0"] = "specific single age";
	genres["4.2.1.1"] = "age 4-7";
	genres["4.2.1.2"] = "age 8-13";
	genres["4.2.1.3"] = "age 14-15";
	genres["4.2.1.4"] = "age 0-3";
	genres["4.2.2"] = "Adults";
	genres["4.2.2.1"] = "age 16-17";
	genres["4.2.2.2"] = "age 18-24";
	genres["4.2.2.3"] = "age 25-34";
	genres["4.2.2.4"] = "age 35-44";
	genres["4.2.2.5"] = "age 45-54";
	genres["4.2.2.6"] = "age 55-64";
	genres["4.2.2.7"] = "age 65+";
	genres["4.2.2.8"] = "specific single age";
	genres["4.2.3"] = "Adults";
	genres["4.2.3.1"] = "Age 25-34";
	genres["4.2.3.2"] = "Age 35-44";
	genres["4.2.3.3"] = "Age 45-54";
	genres["4.2.3.4"] = "Age 55-64";
	genres["4.2.3.5"] = "Age 65+";
	genres["4.2.3.6"] = "Specific single age";
	genres["4.2.4"] = "All ages";
	genres["4.3"] = "AddToCart";
	genres["4.3.1"] = "Ethnic";
	genres["4.3.1.1"] = "Immigrant groups";
	genres["4.3.1.2"] = "Indigineous";
	genres["4.3.2"] = "Religious";
	genres["4.5"] = "OTHER SPECIAL INTEREST/OCCUPATIONAL GROUPS";
	genres["4.6"] = "GENDER";
	genres["4.6.1"] = "Primarily for males";
	genres["4.6.2"] = "Primarily for females";
	genres["4.6.3"] = "For males and females";
	genres["4.7"] = "GEOGRAPHICAL";
	genres["4.7.1"] = "Universal";
	genres["4.7.2"] = "Continental";
	genres["4.7.3"] = "National";
	genres["4.7.4"] = "Regional";
	genres["4.7.5"] = "Local";
	genres["4.7.6"] = "Multinational";
	genres["4.8"] = "EDUCATION STANDARD";
	genres["4.8.1"] = "Primary";
	genres["4.8.2"] = "Secondary";
	genres["4.8.3"] = "Tertiary";
	genres["4.8.4"] = "Post Graduate/Life Long Learning";
	genres["4.9"] = "LIFESTYLE STAGES";
	genres["4.9.1"] = "Single";
	genres["4.9.2"] = "Couple";
	genres["4.9.3"] = "Family with Children 0-3";
	genres["4.9.4"] = "Family with Children 4-7";
	genres["4.9.5"] = "Family with Children 8-15";
	genres["4.9.6"] = "Family with Children 16+";
	genres["4.9.7"] = "Empty Nester";
	genres["4.9.8"] = "Retired";
	genres["5"] = "Educational notes";
	genres["5.1"] = "Studio";
	genres["5.1.1"] = "Live";
	genres["5.1.2"] = "As live";
	genres["5.1.3"] = "Edited";
	genres["5.10"] = "Online Distribution";
	genres["5.10.1"] = "Made on location";
	genres["5.10.1.1"] = "Live";
	genres["5.10.1.2"] = "As Live";
	genres["5.10.1.3"] = "Edited";
	genres["5.10.2"] = "Made in studio";
	genres["5.10.2.1"] = "Live";
	genres["5.10.2.2"] = "As Live";
	genres["5.10.2.3"] = "Edited";
	genres["5.10.3"] = "Made on consumer equipment";
	genres["5.10.3.1"] = "Live";
	genres["5.10.3.2"] = "As Live";
	genres["5.10.3.3"] = "Edited";
	genres["5.11"] = "Offline Distribution";
	genres["5.2"] = "Made on Location";
	genres["5.2.1"] = "Live";
	genres["5.2.2"] = "As live";
	genres["5.2.3"] = "Edited";
	genres["5.3"] = "Cinema industry originated";
	genres["5.4"] = "Made on film (but not originating from the cinema industry)";
	genres["5.5"] = "Home video";
	genres["5.6"] = "Multimedia format (I.e. text/computer, etc.)";
	genres["5.7"] = "Cinema";
	genres["5.7.1"] = "Made on location";
	genres["5.7.2"] = "Made in studio";
	genres["5.7.3"] = "Made by the consumer";
	genres["5.8"] = "TV";
	genres["5.8.1"] = "Made on location";
	genres["5.8.1.1"] = "Live";
	genres["5.8.1.2"] = "As Live";
	genres["5.8.1.3"] = "Edited";
	genres["5.8.2"] = "Made in studio";
	genres["5.8.2.1"] = "Live";
	genres["5.8.2.2"] = "As Live";
	genres["5.8.2.3"] = "Edited";
	genres["5.8.3"] = "Made by the consumer";
	genres["5.9"] = "Radio";
	genres["5.9.1"] = "Made on location";
	genres["5.9.1.1"] = "Live";
	genres["5.9.1.2"] = "As Live";
	genres["5.9.1.3"] = "Edited";
	genres["5.9.2"] = "Made in studio";
	genres["5.9.2.1"] = "Live";
	genres["5.9.2.2"] = "As Live";
	genres["5.9.2.3"] = "Edited";
	genres["5.9.3"] = "Made on consumer equipment (home audio)";
	genres["5.9.3.1"] = "Live";
	genres["5.9.3.2"] = "As Live";
	genres["5.9.3.3"] = "Edited";
	genres["7"] = "GroupRecommendation";
	genres["7.0"] = "Proprietary";
	genres["7.1"] = "Linear";
	genres["7.1.1"] = "Audio only";
	genres["7.1.2"] = "Video only";
	genres["7.1.3"] = "Audio and video";
	genres["7.1.4"] = "Multimedia";
	genres["7.1.4.1"] = "Text";
	genres["7.1.4.2"] = "Graphics";
	genres["7.1.4.3"] = "Application";
	genres["7.1.5"] = "Data";
	genres["7.2"] = "Non Linear";
	genres["7.2.1"] = "Audio only";
	genres["7.2.2"] = "Video only";
	genres["7.2.3"] = "Audio and video";
	genres["7.2.4"] = "Multimedia";
	genres["7.2.4.1"] = "Text";
	genres["7.2.4.2"] = "Graphics";
	genres["7.2.4.3"] = "Application";
	genres["7.2.5"] = "Data";
	genres["7.3"] = "AUDIO VIDEO ENHANCEMENTS";
	genres["7.3.1"] = "Linear with non-sync";
	genres["7.3.10"] = "Linear broadcast with online insertions";
	genres["7.3.11"] = "Other";
	genres["7.3.2"] = "Linear with sync";
	genres["7.3.3"] = "Multi stream audio";
	genres["7.3.4"] = "Multi stream video";
	genres["7.3.5"] = "Non-linear one stream av show";
	genres["7.3.6"] = "Non-linear multi stream";
	genres["7.3.7"] = "Hybrid NVOD";
	genres["7.3.8"] = "Mix and match";
	genres["7.3.9"] = "Parallel 'layer controlled' audio or video support";
	genres["8"] = "Commercial advert";
	genres["8.0"] = "Proprietary";
	genres["8.1"] = "Alternative";
	genres["8.10"] = "Confrontational";
	genres["8.11"] = "Contemporary";
	genres["8.12"] = "Crazy";
	genres["8.13"] = "Cutting edge";
	genres["8.14"] = "Eclectic";
	genres["8.15"] = "Edifying";
	genres["8.16"] = "Exciting";
	genres["8.17"] = "Fast-moving";
	genres["8.18"] = "Frantic";
	genres["8.19"] = "Fun";
	genres["8.2"] = "Analytical";
	genres["8.20"] = "Gripping";
	genres["8.21"] = "Gritty";
	genres["8.22"] = "Gutsy";
	genres["8.23"] = "Happy";
	genres["8.24"] = "Heart-rending";
	genres["8.25"] = "Heart-warming";
	genres["8.26"] = "Hot";
	genres["8.27"] = "Humorous";
	genres["8.28"] = "Innovative";
	genres["8.29"] = "Insightful";
	genres["8.3"] = "Astonishing";
	genres["8.30"] = "Inspirational";
	genres["8.31"] = "Intriguing";
	genres["8.32"] = "Irreverent";
	genres["8.33"] = "Laid back";
	genres["8.34"] = "Outrageous";
	genres["8.35"] = "Peaceful";
	genres["8.36"] = "Powerful";
	genres["8.37"] = "Practical";
	genres["8.38"] = "Rollercoaster";
	genres["8.39"] = "Romantic";
	genres["8.4"] = "Ambitious";
	genres["8.40"] = "Rousing";
	genres["8.41"] = "Sad";
	genres["8.42"] = "Satirical";
	genres["8.43"] = "Serious";
	genres["8.44"] = "Sexy";
	genres["8.45"] = "Shocking";
	genres["8.46"] = "Silly";
	genres["8.47"] = "Spooky";
	genres["8.48"] = "Stunning";
	genres["8.49"] = "Stylish";
	genres["8.5"] = "Black";
	genres["8.50"] = "Terrifying";
	genres["8.51"] = "Thriller";
	genres["8.52"] = "Violent";
	genres["8.53"] = "Wacky";
	genres["8.6"] = "Breathtaking";
	genres["8.7"] = "Chilling";
	genres["8.8"] = "Coarse";
	genres["8.9"] = "Compelling";
	genres["9"] = "Direct product purchase";
                  
    dir = EPG_SAVE_PATH;
    servicesFilename = dir + "/services.xml";
    loadChannels (servicesFilename);
    saveChannels (servicesFilename);
}

void
EPG::addChannel (const QString & label, uint32_t sid)
{
    if (!sids.contains (label))
      {
	  sids[label] = sid;
	  saveChannels (servicesFilename);
      }
}

void
EPG::select (const QString & chan, const CDateAndTime & d)
{
    if (!sids.contains (chan))
      {
	  return;
      }
    uint32_t sid = sids[chan];
    progs.clear ();
    /* look for the basic profile */
    QString fileName;
    fileName = dir + "/" + QString (epgFilename (d, sid, 1, false).c_str ());
    getFile (basic, fileName);
    /* look for the advanced profile */
    fileName = dir + "/" + QString (epgFilename (d, sid, 1, true).c_str ());
    getFile (advanced, fileName);
    if (progs.count () == 0)
      {
	  return;
      }
}

void
EPG::getFile (CEPGDecoder & epg, const QString & fileName)
{
    QFile file (fileName);
    if (!file.open (IO_ReadOnly))
      {
	  return;
      }
    vector < _BYTE > vecData;
    vecData.resize (file.size ());
    vecData.resize (file.size ());
    file.readBlock ((char *) &vecData.front (), file.size ());
    file.close ();
    epg.decode (vecData);
    QDomNodeList programmes = epg.doc.elementsByTagName ("programme");
    parseDoc (programmes.item (0));
}

void
EPG::parseDoc (const QDomNode & n)
{
    QDomNode l1 = n;
    while (!l1.isNull ())
      {
	  if (l1.nodeName () == "programme")
	    {
		QDomNode l2 = l1.firstChild ();
		uint32_t shortId =
		    l1.toElement ().attribute ("shortId", "0").toInt ();
		CProg p;
		if (progs.contains (shortId))
		    p = progs[shortId];
		while (!l2.isNull ())
		  {
		      if (l2.isElement ())
			{
			    QDomElement e = l2.toElement ();
			    if (e.tagName () == "location")
			      {
				  QDomNode l3 = e.firstChild ();
				  while (!l3.isNull ())
				    {
					if (l3.isElement ())
					  {
					      QDomElement e = l3.toElement ();
					      if (e.tagName () == "time")
						{
						    QString start =
							e.
							attribute
							("actualTime");
						    if (start != "")
						      {
							  p.start =
							      parseStart
							      (start);
							  p.duration =
							      parseDuration
							      (e.
							       attribute
							       ("actualDuration"));
						      }
						    else
						      {
							  start =
							      e.
							      attribute
							      ("time");
							  if (start != "")
							    {
								p.start =
								    parseStart
								    (start);
								p.duration =
								    parseDuration
								    (e.
								     attribute
								     ("duration"));
							    }
						      }
						}
					  }
					l3 = l3.nextSibling ();
				    }
			      }
			    if ((e.tagName () == "mediumName") && (p.name == ""))
				p.name = e.text ();
			    if (e.tagName () == "longName")
				p.name = e.text ();
			    if (e.tagName () == "mediaDescription")
			      {
				  QDomNode l3 = e.firstChild ();
				  while (!l3.isNull ())
				    {
					if (l3.isElement ())
					  {
					      QDomElement e = l3.toElement ();
					      if (e.tagName () ==
						  "shortDescription")
						{
						    p.description = e.text ();
						}
					  }
					l3 = l3.nextSibling ();
				    }
			      }
			    if (e.tagName () == "genre")
			      {
				  QString type = e.attribute ("type", "main");
				  if (type == "main")
				      p.mainGenre =
					  genres[e.attribute ("href")];
				  else if (type == "secondary")
				      p.secondaryGenre =
					  genres[e.attribute ("href")];
				  else if (type == "other")
				      p.otherGenre =
					  genres[e.attribute ("href")];
			      }
			}
		      l2 = l2.nextSibling ();
		  }
		if (shortId != 0)
		  {
		      progs[shortId] = p;
		  }
	    }
	  l1 = l1.nextSibling ();
      }
}

/*
<service>
<serviceID id="e1.ce15.c221.0" />
<shortName>Radio 1</shortName>
<mediumName>BBC Radio 1</mediumName>
</service>
*/

void
EPG::saveChannels (const QString & fileName)
{
    QFile f (fileName);
    if (!f.open (IO_WriteOnly))
      {
	  return;
      }
    QDomDocument doc ("serviceInformation");
    QDomElement root = doc.createElement ("serviceInformation");
    doc.appendChild (root);
    QDomElement ensemble = doc.createElement ("ensemble");
    root.appendChild (ensemble);
    for (QMap < QString, uint32_t >::Iterator i = sids.begin ();
	 i != sids.end (); i++)
      {
	  QDomElement service = doc.createElement ("service");
	  QDomElement serviceID = doc.createElement ("serviceID");
	  serviceID.setAttribute ("id",
				  QString::number (ulong (i.data ()), 16));
	  service.appendChild (serviceID);
	  QDomElement shortName = doc.createElement ("shortName");
	  QDomText text = doc.createTextNode (i.key ());
	  shortName.appendChild (text);
	  service.appendChild (shortName);
	  ensemble.appendChild (service);
      }
    QTextStream stream (&f);
    stream << doc.toString ();
    f.close ();

}

void
EPG::loadChannels (const QString & fileName)
{
    sids.clear ();
    QDomDocument domTree;
    QFile f (fileName);
    if (!f.open (IO_ReadOnly))
      {
	  sids.insert ("BBCWorld Service", 0xE1C238);
	  return;
      }
    if (!domTree.setContent (&f))
      {
	  f.close ();
	  return;
      }
    f.close ();
    QDomNodeList ensembles = domTree.elementsByTagName ("ensemble");
    QDomNode n = ensembles.item (0).firstChild ();
    while (!n.isNull ())
      {
	  if (n.nodeName () == "service")
	    {
		QDomNode e = n.firstChild ();
		QString name, sid;
		while (!e.isNull ())
		  {
		      if (e.isElement ())
			{
			    QDomElement s = e.toElement ();
			    if (s.tagName () == "shortName")
				name = s.text ();
			    if (s.tagName () == "serviceID")
				sid = s.attribute ("id");
			}
		      e = e.nextSibling ();
		  }
		if (name != "")
		  {
		      sids.insert (name, sid.toUInt (NULL, 16));
		  }
	    }
	  n = n.nextSibling ();
      }
}

QString
EPG::parseStart (const QString & start)
{
    QStringList d = QStringList::split ('+', start, true);
    QStringList t = QStringList::split ('T', d[0], true);
    QStringList hms = QStringList::split (':', t[1], true);
    if (hms[2] == "00")
	return hms[0] + ":" + hms[1];
    else
	return t[1];
}

QString
EPG::parseDuration (const QString & duration)
{
    QRegExp r ("[PTHMS]");
    QStringList dur = QStringList::split (r, duration);
    return
	QCString ("").sprintf ("%02u:%02u", dur[0].toInt (), dur[1].toInt ());
}
