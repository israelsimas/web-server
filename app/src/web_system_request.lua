

local selWAN = "PK,ETHAddressIP,ETHNetMask,ETHDefGateway,ETHPriDNS,ETHSecDNS,ETHLinkBandwidthUpload,ETHLinkBandwidthDownload,ETHActivateDHCPClient,ETHSpeed,ETHTrafficEnableSIP,ETHTrafficEnableRTP,ETHTrafficEnableAdmin, ETHQoSEnableLayer3,ETHQoSTypeForSIP,ETHQoSValueForSIP,ETHQoSTypeForRTP,ETHQoSValueForRTP,ETHQoSTypeForAdmin,ETHQoSValueForAdmin,ETHHostname,ETHDomainName,ETHPPPoE,ETHPPPoEUserName,ETHPPPoEPassword,ETHPPPoEDownTime,ETHPPPoEEcoCount,ETHBroadcastLimit,ETHMulticastLimit,Enable8021X,Protocol8021X,User8021X,Password8021X"
  
function exportAutoprovisioning()

  require('db_driver')

  local qTableName = DbDriver.execute("SELECT name FROM sqlite_master WHERE type = 'table';");
  local xml = require('LuaXML')
  local autoprovisioningData = xml.new("autoprovisioning")
  local fields = {}

  for k, v in pairs(qTableName[1].rows) do
    local tblName = string.lower(v.name)
    if (tblName ~= "tab_system_ring") then
      
      local qTableContent

      if (tblName == "tab_net_eth_wan") then
        qTableContent = DbDriver.execute("SELECT "..selWAN.." FROM tab_net_eth_wan;");                
      elseif (tblName == "tab_voip_account") then
        DbDriver.execute("create table tempVoipAcc as select * from TAB_VOIP_ACCOUNT; update tempVoipAcc set AuthPassword='';");
        qTableContent = DbDriver.execute("SELECT * FROM tempVoipAcc;");
        DbDriver.execute("drop table tempVoipAcc;");              
      else
        qTableContent = DbDriver.execute("SELECT * FROM " .. tblName .. ";");
      end

      local xmlTable
      if #qTableContent[1].rows > 0 then
        xmlTable = autoprovisioningData:append(tblName)
      end

      for i, j in pairs(qTableContent[1].rows) do
        local columns = {}
        local pk
        local account
        local toneSource
        local contactId
        local codecType
        local secAccount
        for x, y in pairs(j) do
          if(x == "PK")then
            pk = y
          elseif(x == "Account") then
            account = y
          elseif(x == "SYSToneSource") then
            toneSource = y
          else
            columns[x] = y
          end
        end
        local wrapping_tag

        if account and not wrapping_tag and tblName ~= "tab_phonebook"then
          wrapping_tag = xmlTable:append("account")
        end

        if tblName == "tab_phonebook" or tblName == "tab_net_eth_lan_dhcp" or tblName == "tab_net_redirection" then
          wrapping_tag = xmlTable:append("item")
        end

        if tblName == "tab_security_account" then
          wrapping_tag = xmlTable:append("security")
        end

        if #qTableContent[1].rows > 1 and not wrapping_tag then
          wrapping_tag = xmlTable:append("id")
        end

        if not wrapping_tag then
          wrapping_tag = xmlTable
        end

        if tblName == "tab_security_account" then
          wrapping_tag["user"] = columns["SECAccount"]
          wrapping_tag[1] = columns["SECPassword"]
        else
          if tblName == "tab_phonebook" then
            wrapping_tag:append("Account")[1] = account
          elseif tblName == "tab_system_tone" then
            wrapping_tag["value"] = pk
            wrapping_tag["source"] = toneSource
          elseif account then
            wrapping_tag["value"] = account
          elseif pk and #qTableContent[1].rows > 1 and wrapping_tag[0] ~= "item" then
            wrapping_tag["value"] = pk
          end
          for x1, y1 in pairs(columns) do
            wrapping_tag:append(x1)[1] = y1
          end
        end
      end
    end
  end

  local fieldsData = '<?xml version="1.0" encoding="UTF-8"?>\n' .. tostring(autoprovisioningData):gsub("&apos;","'");
  local file = io.open("/data/autoprov_exported.xml", "w")
  file:write(fieldsData)
  file:close()
end

if (arg[1] == "export_autoprov") then
  exportAutoprovisioning()
elseif (arg[1] == "stop") then
  stop()  
elseif (arg[1] == "restart") then
  restart()
end
