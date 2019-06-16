local xml = require('LuaXML')
 
function exportAutoprovisioning()

  require('db_driver')

  local selWAN = "PK,ETHAddressIP,ETHNetMask,ETHDefGateway,ETHPriDNS,ETHSecDNS,ETHLinkBandwidthUpload,ETHLinkBandwidthDownload,ETHActivateDHCPClient,ETHSpeed,ETHTrafficEnableSIP,ETHTrafficEnableRTP,ETHTrafficEnableAdmin, ETHQoSEnableLayer3,ETHQoSTypeForSIP,ETHQoSValueForSIP,ETHQoSTypeForRTP,ETHQoSValueForRTP,ETHQoSTypeForAdmin,ETHQoSValueForAdmin,ETHHostname,ETHDomainName,ETHPPPoE,ETHPPPoEUserName,ETHPPPoEPassword,ETHPPPoEDownTime,ETHPPPoEEcoCount,ETHBroadcastLimit,ETHMulticastLimit,Enable8021X,Protocol8021X,User8021X,Password8021X"
  local qTableName = DbDriver.execute("SELECT name FROM sqlite_master WHERE type = 'table';");
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


ContactParse = {}

xml_to_contacts_mapping = {sDisplayName="IdentificationName", sOfficeNumber="OfficeNumber", sMobilNumber="MobilePhone", sOtherNumber="OtherNumber", sRing="DistintiveRing", sAccount="Account", sIcon="Icon"}

function ContactParse:parseXMLContact(xmlText,tag)
  
  local contacts = {}
  local xfile = xml.eval(xmlText)
  local group = xfile:find(tag)
  
  for i=1,#group do
    local contact = {}
    for k,v in pairs(group[i]) do
      if xml_to_contacts_mapping[k] ~= nil then
        contact[xml_to_contacts_mapping[k]] = v
      end
    end
    if ( (contact[xml_to_contacts_mapping["sDisplayName"]] ~= nil) and (contact[xml_to_contacts_mapping["sOfficeNumber"]] ~= nil) and (contact[xml_to_contacts_mapping["sRing"]] ~= nil) and (contact[xml_to_contacts_mapping["sAccount"]] ~= nil) ) then
      table.insert(contacts,contact)
    else
      print('Invalid Contact')
    end
  end

  return contacts
end

function ContactParse:parseContacts(file)
  return self:parseXMLContact(file,"group")
end

function ContactParse:parseBlackList(file)
  return self:parseXMLContact(file,"blacklist")
end

ContactGenerator = {}

function ContactGenerator:toXML(contacts,contactsBlacklist)
  local contactData = xml.new("contactData")

  if contacts then
    local group =  contactData:append("group")
    
    for i=1,#contacts do
      
      local contact            = group:append("contact")
      contact["sDisplayName"]  = contacts[i].IdentificationName
      contact["sOfficeNumber"] = contacts[i].OfficeNumber
      contact["sRing"]         = contacts[i].DistintiveRing
      contact["sAccount"]      = contacts[i].Account
      contact["sIcon"]         = contacts[i].Icon
    end

  end
  if contactsBlacklist then
    local blacklist =  contactData:append("blacklist")
    
    for i=1,#contactsBlacklist do

      local contactBlacklist             = blacklist:append("contact")
      contactBlacklist["sDisplayName"]   = contactsBlacklist[i].IdentificationName
      contactBlacklist["sOfficeNumber"]  = contactsBlacklist[i].OfficeNumber
      contactBlacklist["sRing"]          = contactsBlacklist[i].DistintiveRing
      contact["sAccount"]                = contacts[i].Account
    end
  end

  return contactData
end

function exportContacts(export_ids)

  print("exporting contacts ...")
  require('db_driver')
  
  local ids = export_ids:split(",")

  local where_clauses = nil

  for i=1, #ids do
    if where_clauses then
      where_clauses = where_clauses .. " or PK = " .. ids[i]
    else
      where_clauses =  " PK = ".. ids[i]
    end

  end

  local selectSql = "SELECT PK, Account, IndexPhoneBook, IdentificationName, OfficeNumber, DistintiveRing, Icon FROM TAB_PHONEBOOK WHERE " .. where_clauses .. " ORDER BY PK ASC;"

  require('db_driver')
  local cnx = Db:getConnection()
  local contacts = DbDriver.executeSelect(selectSql, cnx)
  cnx:close()

  blacklist = {}

  local contactData = ContactGenerator:toXML(contacts.rows, blacklist)

  local xmlStr = '<?xml version="1.0" encoding="UTF-8"?>\n' .. tostring(contactData);
  local file = io.open("/data/contacts_exported.xml", "w")
  file:write(xmlStr)
  file:close()
end

if (arg[1] == "export_autoprov") then
  exportAutoprovisioning()
elseif (arg[1] == "export_contacts") then
  exportContacts(arg[2])
elseif (arg[1] == "restart") then
  restart()
end
