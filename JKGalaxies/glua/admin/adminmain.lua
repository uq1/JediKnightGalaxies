--[[ ------------------------------------------------
	JKA Galaxies Lua Code
	Admin system
		
	Written by BobaFett
--------------------------------------------------]]

local admins = { }
local ranks = { }

				 
local function GetRank(ply)
	if ply.IsAdmin then
		local account = ply:GetAdminAccount()
		return ranks[admins[account]["rank"]]
	end
	return nil
end

local function UpdateJSON()
end
				 
local function AdmReply(ply, message)
	ply:SendChat("^7Admin System: " .. message)
end

local function Evt_PlayerBegin(ply)
	local k,v
	local ip = ply:GetIP()
	for k,v in pairs(autologins) do
		if v == ip then
			ply.IsAdmin = true
		end
	end
end
hook.Add("PlayerBegin", "AdminHook", Evt_PlayerBegin)

local function Admin_Login(ply, argc, argv)
	local k,v
	if argc ~= 3 then
		-- Invalid number of arguments
		AdmReply(ply, "^1Invalid arguments")
		return
	end
	if ply.IsAdmin then
		-- We are admin, so no need to re-log us
		AdmReply(ply, "^5You are already logged in")
		return
	end
	if admins[argv[1]] ~= nil then
		if admins[argv[1]]["password"] == argv[2] then
			ply.IsAdmin = true
			ply:SetAdminAccount(argv[1])
			AdmReply(ply, "^2Admin login successful")
			return
		end
	end
	AdmReply(ply, "^1Bad username/password")
end

chatcmds.Add("admlogin", Admin_Login)

local function Admin_Logout(ply, argc, argv)
	local k,v
	if ply.IsAdmin then
		ply.IsAdmin = false
		ply:SetAdminAccount(" ")
		AdmReply(ply, "^5You are now logged out")
		return
	end
	chatcmds.Ignore()
end

chatcmds.Add("admlogout", Admin_Logout)

local function Admin_Kick(ply, argc, argv)
	local k,v
	if ply.IsAdmin then
		if argc < 2 then
			AdmReply(ply, "^1Please specify a player to kick")
			return
		end
		local target =  players.GetByArg(argv[1])
		if not target then
			AdmReply(ply, "^1Invalid player specified")
			return
		end
		local reason
		if argc > 2 then
			reason = table.concat(argv," ",2, argc-1)
		else 
			reason = nil
		end
		AdmReply(ply, "^5Player " .. target:GetName() .. " ^5has been kicked")
		target:Kick(reason)
		return
	end
	chatcmds.Ignore()
end

chatcmds.Add("admkick", Admin_Kick)

local function Admin_ChangeDetails(ply, argc, argv)
	if ply.IsAdmin then
		local rank = GetRank(ply)
		if rank["can-changedetails"] == true then
			-- Change our account details then?
			if argc < 2
				AdmReply(ply, "Please specify which details you'd like to change: password")
			end

			if argv[2] == "password" or argv[2] == "Password" then
				-- We want to change the password on this account.
				-- Syntax:
				-- /admchangedetails password <oldpass> <newpass>
				if argc ~= 4
					AdmReply(ply, "Syntax: /admchangedetails password <oldpass> <newpass>")
				else
					local accountname = ply:GetAdminAccount()
					local account = admins[accountname]
					if argv[3] ~= account["password"]
						AdmReply(ply, "Your old password did not match correctly. Please try again.")
					else
						-- kk, change.
					end
				end
			else
				AdmReply(ply, "Invalid details type specified. Valid details: password")
			end
		else
			AdmReply(ply, "^4You don't have permission to perform this action.")
		end
	end
	chatcmds.Ignore()
end

chatcmds.Add("admchangedetails", Admin_ChangeDetails)

local function Admin_Profile(ply, argc, argv)
	if ply.IsAdmin then
		local account = ply:GetAdminAccount()
		AdmReply(ply, "^4You are logged in as " .. account .. " [Rank: " .. admins[account]["rank"] .. "]")
		return
	end
	chatcmds.Ignore()
end

chatcmds.Add("admprofile", Admin_Profile)

--
-- Init admin system stuff
--

local function InitAdminList ( )
	-- Grab our file
	local jRoot = json.RegisterFile("server/adminlist.json")

	if jRoot == nil then
		json.Clear()
		securitylog("Could not register adminlist.json")
		return
	end

	-- Instead of using a global Lua variable, let's load up a list of users
	local adminArray = json.GetObjectItem( jRoot, "admins" )
	if adminArray == nil then
		-- Lol, does not exist.
		json.Clear()
		securitylog("admins array in adminlist.json nonexisting.")
		return
	end

	-- ok, so loop through the objects
	local i
	for i = 0, json.GetArraySize(adminArray)-1 do
		local jObject = json.GetArrayItem( adminArray, i )
		local admin = {}
		local jObjectItem

		jObjectItem = json.GetObjectItem( jObject, "username" )
		admin["username"] = json.ToString( jObjectItem )
		
		jObjectItem = json.GetObjectItem( jObject, "password" )
		admin["password"] = json.ToString( jObjectItem )

		jObjectItem = json.GetObjectItem( jObject, "rank" )
		admin["rank"] = json.ToString( jObjectItem )

		if ranks[admin["rank"]] == nil then
			securitylog("WARNING: admin account " .. admin["username"] .. " pointing to invalid rank " .. admin["rank"])
			print("^3WARNING: admin account " .. admin["username"] .. " pointing to invalid rank " .. admin["rank"])
		end

		admins[admin["username"]] = admin
	end

	admins["numAdmins"] = json.GetArraySize(adminArray)

	-- Make sure to clean up our mess.
	json.Clear()
end

local function InitRanks( )
	-- Grab our file
	local jRoot = json.RegisterFile("server/ranks.json")
	
	if jRoot == nil then
		json.Clear()
		securitylog("Could not register ranklist.json")
		return
	end

	-- Load up the list of ranks
	local rankArray = json.GetObjectItem( jRoot, "ranks" )
	if rankArray == nil then
		-- Lol, does not exist.
		json.Clear()
		securitylog("ranks array in ranks.json nonexisting.")
		return
	end

	-- Loop through objects
	local i
	for i = 0, json.GetArraySize(rankArray)-1 do
		local jObject = json.GetArrayItem( rankArray, i )
		local rank = {}
		local jObjectItem

		jObjectItem = json.GetObjectItem( jObject, "name" )
		rank["name"] = json.ToString( jObjectItem )

		--
		-- INSERT COMMAND-BASED STUFF HERE
		--

		jObjectItem = json.GetObjectItem( jObject, "can-changedetails" )
		rank["can-changedetails"] = json.ToBoolean( jObjectItem )
		
		--
		-- END COMMAND-BASED STUFF
		--

		ranks[rank["name"]] = rank
	end

	ranks["numRanks"] = json.GetArraySize(rankArray)
end

local function InitAdminSys( )
	InitRanks()
	InitAdminList()
end


-- Ok, now init stuff and whatnot
InitAdminSys()