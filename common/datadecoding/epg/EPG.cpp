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
    genres["3.1"] = "NON-FICTION";
    genres["3.1.1"] = "News";
    genres["3.1.1.1"] = "Daily news";
    genres["3.1.1.2"] = "Special news/edition";
    genres["3.1.1.3"] = "Special Reports";
    genres["3.1.1.4"] = "Commentary";
    genres["3.1.1.5"] = "Periodical/General";
    genres["3.1.1.6"] = "National politics/National Assembly";
    genres["3.1.1.7"] = "Economy/Market/Financial/Business";
    genres["3.1.1.8"] = "Foreign/International";
    genres["3.1.1.9"] = "Sports";
    genres["3.1.1.10"] = "Cultural";
    genres["3.1.1.11"] = "Local/regional";
    genres["3.1.1.12"] = "Traffic";
    genres["3.1.1.13"] = "Weather forecasts";
    genres["3.1.1.14"] = "Service information";
    genres["3.1.1.15"] = "Public Affairs";
    genres["3.1.1.16"] = "Current affairs";
    genres["3.1.2"] = "Philosophies of life";
    genres["3.1.2.1"] = "Religion";
    genres["3.1.2.1.1"] = "Buddhism";
    genres["3.1.2.1.2"] = "Hinduism";
    genres["3.1.2.1.3"] = "Christianity";
    genres["3.1.2.1.4"] = "Islam";
    genres["3.1.2.1.5"] = "Judaism";
    genres["3.1.2.1.6"] = "Atheism";
    genres["3.1.2.1.7"] = "Agnosticism";
    genres["3.1.2.1.8"] = "Shintoism";
    genres["3.1.2.2"] = "Non-religious philosophies";
    genres["3.1.2.2.1"] = "Communism";
    genres["3.1.2.2.2"] = "Humanism";
    genres["3.1.2.2.3"] = "Capitalism";
    genres["3.1.2.2.4"] = "Socialism";
    genres["3.1.2.2.5"] = "Libertarianism";
    genres["3.1.2.2.6"] = "Republicanism";
    genres["3.1.3"] = "General non-fiction";
    genres["3.1.3.1"] = "Political";
    genres["3.1.3.2"] = "Social";
    genres["3.1.3.3"] = "Economic";
    genres["3.1.3.4"] = "Legal";
    genres["3.1.3.5"] = "Finance";
    genres["3.1.3.6"] = "Education";
    genres["3.1.3.7"] = "International affairs";
    genres["3.1.3.8"] = "Military/Defence";
    genres["3.1.4"] = "Arts & Media";
    genres["3.1.4.1"] = "Music";
    genres["3.1.4.2"] = "Dance";
    genres["3.1.4.3"] = "Theatre";
    genres["3.1.4.4"] = "Opera";
    genres["3.1.4.5"] = "Cinema";
    genres["3.1.4.6"] = "Advertising";
    genres["3.1.4.7"] = "Press";
    genres["3.1.4.8"] = "Plastic Arts";
    genres["3.1.4.9"] = "Fine arts";
    genres["3.1.4.10"] = "Experimental arts";
    genres["3.1.4.11"] = "Architecture";
    genres["3.1.4.12"] = "Showbiz";
    genres["3.1.4.13"] = "Television";
    genres["3.1.4.14"] = "Radio";
    genres["3.1.4.15"] = "New media";
    genres["3.1.5"] = "Humanities";
    genres["3.1.5.1"] = "Literature";
    genres["3.1.5.2"] = "Languages";
    genres["3.1.5.3"] = "History";
    genres["3.1.5.4"] = "Culture/tradition/anthropology/Ethnicstudies";
    genres["3.1.5.5"] = "War/Conflict";
    genres["3.1.6"] = "Sciences";
    genres["3.1.6.1"] = "Applied sciences";
    genres["3.1.6.2"] = "Nature/natural sciences";
    genres["3.1.6.3"] = "Animals/Wildlife";
    genres["3.1.6.4"] = "Environment/geography";
    genres["3.1.6.5"] = "Space/Universe";
    genres["3.1.6.6"] = "Physical sciences";
    genres["3.1.6.7"] = "Medicine";
    genres["3.1.6.8"] = "Technology";
    genres["3.1.6.9"] = "Physiology";
    genres["3.1.6.10"] = "Psychology";
    genres["3.1.6.11"] = "Social";
    genres["3.1.6.12"] = "Spiritual";
    genres["3.1.6.13"] = "Mathematics";
    genres["3.1.6.14"] = "Archaeology";
    genres["3.1.7"] = "Human interest";
    genres["3.1.7.1"] = "Reality";
    genres["3.1.7.2"] = "Society/show business/Gossip";
    genres["3.1.7.3"] = "Biography/notable personalities";
    genres["3.1.7.4"] = "Personal problems";
    genres["3.1.7.5"] = "Investigative journalism";
    genres["3.1.7.6"] = "Museums";
    genres["3.1.7.7"] = "Religious buildings";
    genres["3.1.8"] = "Transport and Communications";
    genres["3.1.8.1"] = "Air";
    genres["3.1.8.2"] = "Land";
    genres["3.1.8.3"] = "Sea";
    genres["3.1.8.4"] = "Space";
    genres["3.1.9"] = "Events";
    genres["3.1.9.1"] = "Anniversary";
    genres["3.1.9.2"] = "Fair";
    genres["3.1.9.3"] = "Tradeshow";
    genres["3.1.9.4"] = "Musical";
    genres["3.1.9.5"] = "Exhibition";
    genres["3.2"] = "SPORTS";
    genres["3.2.1"] = "Athletics";
    genres["3.2.1.1"] = "Field";
    genres["3.2.1.2"] = "Track";
    genres["3.2.1.3"] = "Combined athletics";
    genres["3.2.1.4"] = "Running";
    genres["3.2.1.5"] = "Cross-country";
    genres["3.2.1.6"] = "Triathlon";
    genres["3.2.2"] = "Cycling/bicycle";
    genres["3.2.2.1"] = "Mountainbike";
    genres["3.2.2.2"] = "Bicross";
    genres["3.2.2.3"] = "Indoor cycling";
    genres["3.2.2.4"] = "Road Cycling";
    genres["3.2.3"] = "Team sports";
    genres["3.2.3.1"] = "Football (american)";
    genres["3.2.3.2"] = "Football (australian)";
    genres["3.2.3.3"] = "Football (gaelic)";
    genres["3.2.3.4"] = "Football (indoor)";
    genres["3.2.3.5"] = "Beach soccer";
    genres["3.2.3.6"] = "Bandy";
    genres["3.2.3.7"] = "Baseball";
    genres["3.2.3.8"] = "Basketball";
    genres["3.2.3.9"] = "Cricket";
    genres["3.2.3.10"] = "Croquet";
    genres["3.2.3.11"] = "Faustball";
    genres["3.2.3.12"] = "Football (soccer)";
    genres["3.2.3.13"] = "Handball";
    genres["3.2.3.14"] = "Hockey";
    genres["3.2.3.15"] = "Korfball";
    genres["3.2.3.16"] = "Lacrosse";
    genres["3.2.3.17"] = "Netball";
    genres["3.2.3.18"] = "Roller skating";
    genres["3.2.3.19"] = "Rugby";
    genres["3.2.3.19.1"] = "Rugby union";
    genres["3.2.3.19.2"] = "Rugby leage";
    genres["3.2.3.20"] = "Softball";
    genres["3.2.3.21"] = "Volleyball";
    genres["3.2.3.22"] = "Beach volley";
    genres["3.2.3.23"] = "Hurling";
    genres["3.2.3.24"] = "Flying Disc/ Frisbee";
    genres["3.2.4"] = "Racket sports";
    genres["3.2.4.1"] = "Badmington";
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
    genres["3.2.6.2"] = "Yatching";
    genres["3.2.6.3"] = "Canoeing";
    genres["3.2.6.4"] = "Diving";
    genres["3.2.6.5"] = "Fishing";
    genres["3.2.6.6"] = "Polo";
    genres["3.2.6.7"] = "Rowing";
    genres["3.2.6.8"] = "Sailing";
    genres["3.2.6.9"] = "Sub-aquatics";
    genres["3.2.6.10"] = "Surfing";
    genres["3.2.6.11"] = "Swimming";
    genres["3.2.6.12"] = "Water polo";
    genres["3.2.6.13"] = "Water skiing";
    genres["3.2.6.14"] = "Windsurfing";
    genres["3.2.7"] = "Winter sports";
    genres["3.2.7.1"] = "Bobsleigh/tobogganing";
    genres["3.2.7.2"] = "Curling";
    genres["3.2.7.3"] = "Ice-hockey";
    genres["3.2.7.4"] = "Ice-skating";
    genres["3.2.7.5"] = "Luge";
    genres["3.2.7.6"] = "Skating";
    genres["3.2.7.7"] = "Skibob";
    genres["3.2.7.8"] = "Skiing";
    genres["3.2.7.9"] = "Sleddog";
    genres["3.2.7.10"] = "Snowboarding";
    genres["3.2.7.11"] = "Alpine skiing";
    genres["3.2.7.12"] = "Freestyle skiing";
    genres["3.2.7.13"] = "Iinline skating";
    genres["3.2.7.14"] = "Nordic skiing";
    genres["3.2.7.15"] = "Ski jumping";
    genres["3.2.7.16"] = "Speed skating";
    genres["3.2.7.17"] = "Figure skating";
    genres["3.2.7.18"] = "Ice-Dance";
    genres["3.2.7.19"] = "Marathon";
    genres["3.2.7.20"] = "Short-track";
    genres["3.2.7.21"] = "Biathlon";
    genres["3.2.8"] = "Motor sports";
    genres["3.2.8.1"] = "Motor/auto racing";
    genres["3.2.8.2"] = "Motor boating/motor racing";
    genres["3.2.8.3"] = "Motor cycling";
    genres["3.2.8.4"] = "Formula 1";
    genres["3.2.8.5"] = "Indy car";
    genres["3.2.8.6"] = "Karting";
    genres["3.2.8.7"] = "Rally";
    genres["3.2.8.8"] = "Trucking";
    genres["3.2.8.9"] = "Tractor Pulling";
    genres["3.2.8.10"] = "Auto racing";
    genres["3.2.9"] = "'Social' sports";
    genres["3.2.9.1"] = "Billiards";
    genres["3.2.9.2"] = "Boules";
    genres["3.2.9.3"] = "Bowling";
    genres["3.2.9.4"] = "Chess";
    genres["3.2.9.5"] = "Dance sport";
    genres["3.2.9.6"] = "Darts";
    genres["3.2.9.7"] = "Pool";
    genres["3.2.9.8"] = "Snooker";
    genres["3.2.9.9"] = "Tug-of-war";
    genres["3.2.9.10"] = "Balle Pelote";
    genres["3.2.9.11"] = "Basque Pelote";
    genres["3.2.9.12"] = "Trickshot";
    genres["3.2.10"] = "Gymnastics";
    genres["3.2.10.1"] = "Assymetric bars";
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
    genres["3.2.12.2"] = "Extreme sports";
    genres["3.2.12.3"] = "Mountaineering";
    genres["3.2.12.4"] = "Climbing";
    genres["3.2.12.5"] = "Orienteering";
    genres["3.2.12.6"] = "Shooting";
    genres["3.2.12.7"] = "Sport acrobatics";
    genres["3.2.12.8"] = "Rafting";
    genres["3.2.13"] = "Strength-based sports";
    genres["3.2.13.1"] = "Body-building";
    genres["3.2.13.2"] = "Boxing";
    genres["3.2.13.3"] = "Combative sports";
    genres["3.2.13.4"] = "Power-lifting";
    genres["3.2.13.5"] = "Weight-lifting";
    genres["3.2.13.6"] = "Wrestling";
    genres["3.2.14"] = "Air sports";
    genres["3.2.14.1"] = "Ballooning";
    genres["3.2.14.2"] = "Hang gliding";
    genres["3.2.14.3"] = "Sky diving";
    genres["3.2.14.4"] = "Delta-plane";
    genres["3.2.14.5"] = "Parachuting";
    genres["3.2.14.6"] = "Kiting";
    genres["3.2.14.7"] = "Aeronautics";
    genres["3.2.14.8"] = "Gliding";
    genres["3.2.14.9"] = "Flying";
    genres["3.2.14.10"] = "Aerobatics";
    genres["3.2.15"] = "Golf";
    genres["3.2.16"] = "Fencing";
    genres["3.2.17"] = "Dog racing";
    genres["3.2.18"] = "Casting";
    genres["3.2.19"] = "Maccabi";
    genres["3.2.20"] = "Modern Pentathlon";
    genres["3.2.21"] = "Sombo";
    genres["3.3"] = "LEISURE/HOBBY";
    genres["3.3.1"] = "Do-it-yourself";
    genres["3.3.2"] = "Cookery";
    genres["3.3.3"] = "Gardening";
    genres["3.3.4"] = "Travel/Tourism";
    genres["3.3.5"] = "Adventure/Expeditions";
    genres["3.3.6"] = "Fishing";
    genres["3.3.7"] = "Outdoor";
    genres["3.3.8"] = "Pet";
    genres["3.3.9"] = "Craft/Handicraft";
    genres["3.3.10"] = "Art";
    genres["3.3.11"] = "Music";
    genres["3.3.12"] = "Board Games";
    genres["3.3.13"] = "Computer Games";
    genres["3.3.14"] = "Card Cames";
    genres["3.3.15"] = "Fitness / Keep-fit";
    genres["3.3.16"] = "Personal health";
    genres["3.3.17"] = "Car";
    genres["3.3.18"] = "Motorcycle  /Motoring";
    genres["3.3.19"] = "Fashion";
    genres["3.3.20"] = "Life/ House Keeping/Lifestyle";
    genres["3.3.21"] = "Technology/Computing";
    genres["3.3.22"] = "Gaming";
    genres["3.3.23"] = "Shopping";
    genres["3.3.24"] = "Adult";
    genres["3.3.25"] = "Road safety";
    genres["3.3.26"] = "Consumer advice";
    genres["3.3.27"] = "Employment Advice";
    genres["3.3.28"] = "Boating";
    genres["3.3.29"] = "Parenting";
    genres["3.3.30"] = "Self-help";
    genres["3.3.31"] = "Collectibles";
    genres["3.3.32"] = "Jewellery";
    genres["3.3.33"] = "Beauty";
    genres["3.3.34"] = "Aviation";
    genres["3.4"] = "FICTION";
    genres["3.4.1"] = "General light drama";
    genres["3.4.2"] = "Soap";
    genres["3.4.2.1"] = "Soap Opera";
    genres["3.4.2.2"] = "Soap Special";
    genres["3.4.2.3"] = "Soap Talk";
    genres["3.4.3"] = "Romance";
    genres["3.4.4"] = "Legal Melodrama";
    genres["3.4.5"] = "Medical melodrama";
    genres["3.4.6"] = "Action";
    genres["3.4.6.1"] = "Adventure";
    genres["3.4.6.2"] = "Disaster";
    genres["3.4.6.3"] = "Mystery";
    genres["3.4.6.4"] = "Detective";
    genres["3.4.6.5"] = "Historical/epic";
    genres["3.4.6.6"] = "Horror";
    genres["3.4.6.7"] = "Science fiction";
    genres["3.4.6.8"] = "War";
    genres["3.4.6.9"] = "Western";
    genres["3.4.6.10"] = "Thriller";
    genres["3.4.6.11"] = "Sports";
    genres["3.4.6.12"] = "Martial arts";
    genres["3.4.6.13"] = "Epic";
    genres["3.4.7"] = "Fantasy/Fairy tale";
    genres["3.4.8"] = "Erotica";
    genres["3.4.9"] = "Drama based on real events (docudrama)";
    genres["3.4.10"] = "Musical";
    genres["3.4.11"] = "Comedy";
    genres["3.4.12"] = "Effect Movies";
    genres["3.4.13"] = "Classical drama";
    genres["3.4.14"] = "Period drama";
    genres["3.4.15"] = "Contemporary drama";
    genres["3.4.16"] = "Religious";
    genres["3.4.17"] = "Poems / Stories";
    genres["3.4.18"] = "biography,";
    genres["3.4.19"] = "psychological drama";
    genres["3.5"] = "AMUSEMENT";
    genres["3.5.1"] = "Game show";
    genres["3.5.2"] = "Quiz/Contest";
    genres["3.5.3"] = "Variety Show";
    genres["3.5.4"] = "Surprise show";
    genres["3.5.5"] = "Reality show";
    genres["3.5.6"] = "Candid camera";
    genres["3.5.7"] = "Comedy";
    genres["3.5.7.1"] = "Broken comedy";
    genres["3.5.7.2"] = "Romantic comedy";
    genres["3.5.7.3"] = "Sitcom";
    genres["3.5.7.4"] = "Satire";
    genres["3.5.9"] = "Humour";
    genres["3.5.10"] = "Magic/hypnotism";
    genres["3.5.11"] = "Circus";
    genres["3.5.12"] = "Dating show";
    genres["3.5.13"] = "Bullfighting";
    genres["3.5.14"] = "Rodeo";
    genres["3.5.15"] = "Airshow";
    genres["3.6"] = "MUSIC";
    genres["3.6.1"] = "Classical music";
    genres["3.6.1.1"] = "Early";
    genres["3.6.1.2"] = "Classical";
    genres["3.6.1.3"] = "Romantic";
    genres["3.6.1.4"] = "Contemporary";
    genres["3.6.1.5"] = "Light classical";
    genres["3.6.1.6"] = "Middle Ages";
    genres["3.6.1.7"] = "Renaissance";
    genres["3.6.1.8"] = "Baroque";
    genres["3.6.1.9"] = "Opera";
    genres["3.6.1.10"] = "Solo instruments (e.g. Piano)";
    genres["3.6.1.11"] = "Chamber";
    genres["3.6.1.12"] = "Symphonic";
    genres["3.6.1.13"] = "Vocal";
    genres["3.6.1.14"] = "Choral";
    genres["3.6.2"] = "Jazz";
    genres["3.6.2.1"] = "New Orleans/early jazz";
    genres["3.6.2.2"] = "Big band/Swing/Dixie";
    genres["3.6.2.3"] = "Blues/soul jazz";
    genres["3.6.2.4"] = "Bop/hard bop/bebop";
    genres["3.6.2.5"] = "Traditional/smooth";
    genres["3.6.2.6"] = "Cool/free";
    genres["3.6.2.7"] = "Modern/Acid/Avant-garde";
    genres["3.6.2.8"] = "Latin & World jazz";
    genres["3.6.2.9"] = "Pop jazz/jazz funk";
    genres["3.6.2.10"] = "Acid jazz / fusion";
    genres["3.6.3"] = "Background music";
    genres["3.6.3.1"] = "Middle-of-the-road";
    genres["3.6.3.2"] = "Easy listening";
    genres["3.6.3.3"] = "Ambient";
    genres["3.6.3.4"] = "Mood music";
    genres["3.6.3.5"] = "Oldies";
    genres["3.6.3.6"] = "Love songs";
    genres["3.6.3.7"] = "Dance hall";
    genres["3.6.3.8"] = "Soundtrack";
    genres["3.6.3.9"] = "Trailer";
    genres["3.6.3.10"] = "Showtunes";
    genres["3.6.3.11"] = "TV";
    genres["3.6.3.12"] = "Cabaret";
    genres["3.6.3.13"] = "Instrumental";
    genres["3.6.3.14"] = "Sound clip";
    genres["3.6.3.15"] = "Retro";
    genres["3.6.4"] = "Pop-rock";
    genres["3.6.4.1"] = "Pop";
    genres["3.6.4.2"] = "Chanson/ballad";
    genres["3.6.4.3"] = "Traditional rock and roll";
    genres["3.6.4.4"] = "Soft/slow rock";
    genres["3.6.4.5"] = "Classic/dance/pop-rock";
    genres["3.6.4.6"] = "Folk";
    genres["3.6.4.7"] = "Punk/funk rock";
    genres["3.6.4.8"] = "New Age";
    genres["3.6.4.9"] = "Instrumental/Band/symphonic rock/jam bands";
    genres["3.6.4.10"] = "Progressive/alternative/indie/experimental/art-rock";
    genres["3.6.4.11"] = "Seasonal/holiday";
    genres["3.6.4.12"] = "Japanese  pop-rock";
    genres["3.6.4.13"] = "Karaoke / singing contests";
    genres["3.6.5"] = "Blues/Rhythm and blues/Soul/Gospel";
    genres["3.6.5.1"] = "Blues";
    genres["3.6.5.2"] = "R & B";
    genres["3.6.5.3"] = "Soul";
    genres["3.6.5.4"] = "Gospel";
    genres["3.6.6"] = "Country and Western";
    genres["3.6.7"] = "Rap/Hip Hop/Reggae";
    genres["3.6.7.1"] = "Rap/Christian rap";
    genres["3.6.7.2"] = "Hip Hop/Trip-Hop";
    genres["3.6.7.3"] = "Reggae";
    genres["3.6.7.4"] = "Ska/Gangsta";
    genres["3.6.8"] = "Electronic/Club/Urban/Dance";
    genres["3.6.8.1"] = "Acid/Punk/Acid Punk";
    genres["3.6.8.2"] = "Disco";
    genres["3.6.8.3"] = "Techno/Euro-Techno/Techno-Industrial/Industrial";
    genres["3.6.8.4"] = "House/Techno House";
    genres["3.6.8.5"] = "Rave";
    genres["3.6.8.6"] = "Jungle/tribal";
    genres["3.6.8.7"] = "Trance";
    genres["3.6.8.8"] = "Punk";
    genres["3.6.8.9"] = "Garage/psychadelic";
    genres["3.6.8.10"] = "Metal/Death metal/Pop metal";
    genres["3.6.8.11"] = "Drum and Bass";
    genres["3.6.8.12"] = "Pranks";
    genres["3.6.8.13"] = "Grunge";
    genres["3.6.8.14"] = "Dance/dance-pop";
    genres["3.6.9"] = "World/Traditional/Ethnic/Folk music";
    genres["3.6.9.1"] = "Africa";
    genres["3.6.9.2"] = "Asia";
    genres["3.6.9.3"] = "Australia/Oceania";
    genres["3.6.9.4"] = "Caribbean";
    genres["3.6.9.5"] = "Europe";
    genres["3.6.9.6"] = "Latin America";
    genres["3.6.9.7"] = "Middle East";
    genres["3.6.9.8"] = "North America";
    genres["3.6.10"] = "Hit-Chart/Song Requests";
    genres["3.6.11"] = "Children's Songs";
    genres["3.6.12"] = "Event music";
    genres["3.6.12.1"] = "wedding";
    genres["3.6.12.2"] = "sports,";
    genres["3.6.12.3"] = "Ceremonial/Chants";
    genres["3.6.13"] = "spoken";
    genres["3.6.14"] = "Dance";
    genres["3.6.14.1"] = "Ballet";
    genres["3.6.14.2"] = "Tap";
    genres["3.6.14.3"] = "Modern";
    genres["3.6.14.4"] = "Classical";
    genres["3.7"] = "INTERACTIVE GAMES";
    genres["3.7.1"] = "Content games categories";
    genres["3.7.1.1"] = "Action";
    genres["3.7.1.2"] = "Adventure";
    genres["3.7.1.3"] = "Fighting";
    genres["3.7.1.4"] = "Online";
    genres["3.7.1.5"] = "Platform";
    genres["3.7.1.6"] = "Puzzle";
    genres["3.7.1.7"] = "RPG/ MUDs";
    genres["3.7.1.8"] = "Racing";
    genres["3.7.1.9"] = "Simulation";
    genres["3.7.1.10"] = "Sports";
    genres["3.7.1.11"] = "Strategy";
    genres["3.7.1.12"] = "Wrestling";
    genres["3.7.1.13"] = "Classic/Retro";
    genres["3.7.2"] = "STYLE";
    genres["3.7.2.1"] = "Logic based";
    genres["3.7.2.2"] = "Word games";
    genres["3.7.2.3"] = "Positional";
    genres["3.7.2.4"] = "Board games";
    genres["3.7.2.5"] = "Text environments";
    genres["3.7.2.6"] = "Dynamic 2D/3D graphics";
    genres["3.7.2.7"] = "Non-linear video";

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
    QRegExp r ("[TPHMS]");
    QStringList dur = QStringList::split (r, duration);
    return
	QCString ("").sprintf ("%02u:%02u", dur[1].toInt (), dur[2].toInt ());
}
