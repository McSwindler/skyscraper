/***************************************************************************
 *            emumovies.cpp
 *
 *  Wed Jun 18 12:00:00 CEST 2017
 *  Copyright 2017 Lars Muldjord
 *  muldjordlars@gmail.com
 ****************************************************************************/
/*
 *  This file is part of skyscraper.
 *
 *  skyscraper is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  skyscraper is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with skyscraper; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#include <QFileInfo>
#include <QProcess>

#include "emumovies.h"
#include "strtools.h"
#include "crc32.h"

EmuMovies::EmuMovies(Settings *config) : AbstractScraper(config)
{
  connect(&manager, &NetComm::dataReady, &q, &QEventLoop::quit);

  baseUrl = "https://api.gamesdbase.com";

  fetchOrder.append(SCREENSHOT);
  fetchOrder.append(COVER);
  fetchOrder.append(WHEEL);
  fetchOrder.append(MARQUEE);
  fetchOrder.append(VIDEO);

  login();
}

void EmuMovies::login()
{
  QString loginUrl = baseUrl + "/login.aspx";
  QString postData = "user=" + config->user + "&api=" + config->password + "&product=D42BE62CA8E3A4BDB9CEBDD328D7E726D0E6";

  manager.request(loginUrl, postData);
  q.exec();
  data = manager.getData();

  xmlDoc.setContent(data);

  session = xmlDoc.elementsByTagName("Results").at(0).toElement().elementsByTagName("Result").at(0).toElement().attribute("Session");

}

void EmuMovies::getSearchResults(QList<GameEntry> &gameEntries,
				     QString searchName, QString)
{
  QString platformId = getPlatformId(config->platform);
  if(platformId == "na") {
    reqRemaining = 0;
    printf("\033[0;31mPlatform not supported by EmuMovies or it hasn't yet been included in Skyscraper for this module...\033[0m\n");
    return;
  }

  GameEntry game;
  game.title = searchName;
  game.platform = platformId;

  gameEntries.append(game);
}

void EmuMovies::getGameData(GameEntry &game)
{
  for(int a = 0; a < fetchOrder.length(); ++a) {
    switch(fetchOrder.at(a)) {
    case COVER:
      getCover(game);
      break;
    case SCREENSHOT:
      getScreenshot(game);
      break;
    case WHEEL:
      getWheel(game);
      break;
    case MARQUEE:
      getMarquee(game);
      break;
    case VIDEO:
      if(config->videos) {
	      getVideo(game);
      }
      break;
    default:
      ;
    }
  }
}

QString EmuMovies::getMediaUrl(GameEntry &game, QString mediaType)
{
  QString platformId = getPlatformId(config->platform);
  QString searchUrl = baseUrl + "/searchbulk.aspx?system=" + platformId + "&media=" + mediaType + "&sessionid=" + session;
  QString postData = "FileNames=" + QUrl::toPercentEncoding(game.title);

  manager.request(searchUrl, postData);
  q.exec();
  data = manager.getData();

  xmlDoc.setContent(data);

  QDomNodeList resultList = xmlDoc.elementsByTagName("Results").at(0).toElement().elementsByTagName("Result");
  if(!resultList.isEmpty()) {
    QDomElement result = resultList.at(0).toElement();
    if(result.attribute("Found") == "True") {
      return result.attribute("URL");
    }
  }

  return "";
}

void EmuMovies::getCover(GameEntry &game)
{
  QString url = getMediaUrl(game, "Box");
  if(!url.isEmpty()) {
    manager.request(url);
    q.exec();
    QImage image;
    if(image.loadFromData(manager.getData())) {
      game.coverData = image;
    }
  }
}

void EmuMovies::getScreenshot(GameEntry &game)
{
  QString url = getMediaUrl(game, "Snap");
  if(!url.isEmpty()) {
    manager.request(url);
    q.exec();
    QImage image;
    if(image.loadFromData(manager.getData())) {
      game.screenshotData = image;
    }
  }
}

void EmuMovies::getWheel(GameEntry &game)
{
  QString url = getMediaUrl(game, "Logos");
  if(!url.isEmpty()) {
    manager.request(url);
    q.exec();
    QImage image;
    if(image.loadFromData(manager.getData())) {
      game.wheelData = image;
    }
  }
}

void EmuMovies::getMarquee(GameEntry &game)
{
  QString url = getMediaUrl(game, "Banner");
  if(!url.isEmpty()) {
    manager.request(url);
    q.exec();
    QImage image;
    if(image.loadFromData(manager.getData())) {
      game.marqueeData = image;
    }
  }
}

void EmuMovies::getVideo(GameEntry &game)
{
  QString url = getMediaUrl(game, "Video_MP4_HI_QUAL");
  if(!url.isEmpty()) {
    manager.request(url);
    q.exec();
    game.videoData = manager.getData();
    QByteArray contentType = manager.getContentType();
    if(contentType.contains("video/") && game.videoData.size() > 4096) {
      game.videoFormat = "mp4";
    } else {
      game.videoData = "";
    }
  }
}

QString EmuMovies::getCompareTitle(QFileInfo info)
{
  return info.fileName();
}

QList<QString> EmuMovies::getSearchNames(const QFileInfo &info)
{
  QList<QString> searchNames;
  searchNames.append(info.fileName());

  return searchNames;
}
  
QString EmuMovies::getPlatformId(const QString platform)
{
  if(platform == "3do") {
    return "Panasonic_3DO";
  } else if(platform == "amiga") {
    return "Commodore_Amiga";
  } else if(platform == "aga") {
    return "Commodore_Amiga";
  } else if(platform == "cd32") {
    return "Commodore_Amiga_CD32";
  } else if(platform == "cdtv") {
    return "Commodore_CDTV";
  } else if(platform == "amstradcpc") {
    return "Amstrad_CPC";
  } else if(platform == "apple2") {
    return "Apple_II";
  } else if(platform == "arcade") {
    return "ArcadePC";
  } else if(platform == "atari800") {
    return "Atari_5200";
  } else if(platform == "atari2600") {
    return "Atari_2600";
  } else if(platform == "atari5200") {
    return "Atari_5200";
  } else if(platform == "atari7800") {
    return "Atari_7800";
  } else if(platform == "atarijaguar") {
    return "Atari_Jaguar";
  } else if(platform == "atarilynx") {
    return "Atari_Lynx";
  } else if(platform == "atarist") {
    return "Atari_ST";
  } else if(platform == "c16") {
    return "Commodore_16";
  } else if(platform == "c64") {
    return "Commodore_64";
  } else if(platform == "c128") {
    return "Commodore_128";
  } else if(platform == "coco") {
    return "Tandy_TRS_80_CoCo";
  } else if(platform == "coleco") {
    return "Coleco_Vision";
  } else if(platform == "daphne") {
    return "Daphne";
  } else if(platform == "dragon32") {
    return "Dragon_32_64";
  } else if(platform == "dreamcast") {
    return "Sega_Dreamcast";
  } else if(platform == "fba") {
    return "Final_Burn_Alpha";
  } else if(platform == "fds") {
    return "Nintendo_Famicom_Disk_System";
  } else if(platform == "gameandwatch") {
    return "na";
  } else if(platform == "gamegear") {
    return "Sega_Game_Gear";
  } else if(platform == "gb") {
    return "Nintendo_Game_Boy";
  } else if(platform == "gba") {
    return "Nintendo_Game_Boy_Advance";
  } else if(platform == "gbc") {
    return "Nintendo_Game_Boy_Color";
  } else if(platform == "gc") {
    return "Nintendo_GameCube";
  } else if(platform == "genesis") {
    return "Sega_Genesis";
  } else if(platform == "intellivision") {
    return "Mattel_Intellivision";
  } else if(platform == "mame-advmame") {
    return "MAME";
  } else if(platform == "mame-libretro") {
    return "MAME";
  } else if(platform == "mame-mame4all") {
    return "MAME";
  } else if(platform == "mastersystem") {
    return "Sega_Master_System";
  } else if(platform == "megacd") {
    return "Sega_CD";
  } else if(platform == "megadrive") {
    return "Sega_Genesis";
  } else if(platform == "msx") {
    return "MSX";
  } else if(platform == "n64") {
    return "Nintendo_N64";
  } else if(platform == "nds") {
    return "Nintendo_DS";
  } else if(platform == "neogeo") {
    return "SNK_Neo_Geo_AES";
  } else if(platform == "nes") {
    return "Nintendo_NES";
  } else if(platform == "ngp") {
    return "SNK_Neo_Geo_Pocket";
  } else if(platform == "ngpc") {
    return "SNK_Neo_Geo_Pocket_Color";
  } else if(platform == "oric") {
    return "Tangerine_Oric";
  } else if(platform == "pc") {
    return "Microsoft_DOS";
  } else if(platform == "pc88") {
    return "NEC_PC_8801";
  } else if(platform == "pcfx") {
    return "NEC_PC_FX";
  } else if(platform == "pcengine") {
    return "NEC_PC_Engine";
  } else if(platform == "ports") {
    return "Microsoft_DOS";
  } else if(platform == "ps2") {
    return "Sony_Playstation_2";
  } else if(platform == "psp") {
    return "Sony_PSP";
  } else if(platform == "psx") {
    return "Sony_Playstation";
  } else if(platform == "saturn") {
    return "Sega_Saturn";
  } else if(platform == "scummvm") {
    return "ScummVM";
  } else if(platform == "sega32x") {
    return "Sega_32X";
  } else if(platform == "segacd") {
    return "Sega_CD";
  } else if(platform == "sg-1000") {
    return "Sega_SG_1000";
  } else if(platform == "snes") {
    return "Nintendo_SNES";
  } else if(platform == "ti99") {
    return "Texas_Instruments_TI_99_4A";
  } else if(platform == "trs-80") {
    return "Tandy_TRS_80";
  } else if(platform == "vectrex") {
    return "GCE_Vectrex";
  } else if(platform == "vic20") {
    return "Commodore_VIC_20";
  } else if(platform == "videopac") {
    return "Philips_Videopac_";
  } else if(platform == "virtualboy") {
    return "Nintendo_Virtual_Boy";
  } else if(platform == "wii") {
    return "Nintendo_Wii";
  } else if(platform == "wonderswan") {
    return "Bandai_WonderSwan";
  } else if(platform == "wonderswancolor") {
    return "Bandai_WonderSwan_Color";
  } else if(platform == "x68000") {
    return "Sharp_X68000";
  } else if(platform == "x1") {
    return "Sharp_X1";
  } else if(platform == "zmachine") {
    return "na";
  } else if(platform == "zx81") {
    return "Sinclair_ZX_81";
  } else if(platform == "zxspectrum") {
    return "Sinclair_ZX_Spectrum";
  }
  return "na";
}
