class Character(object):
    def Init(self,jsobj):
	self.AccountID = jsobj['AccountID']
	self.ProfessionID = jsobj['ProfessionID']
	self.CharacterID = jsobj['CharacterID']
	self.CharName = jsobj['CharName']
	self.Level = jsobj['Level']
	self.Strength = jsobj['Strength']
	self.Intelligence = jsobj['Intelligence']
	self.Magic = jsobj['Magic']
	self.Luck = jsobj['Luck']
	self.Experience = jsobj['Experience']
	self.Gender = jsobj['Gender']
	self.Appr = jsobj['Appr']
	self.Scene = jsobj['Scene']
	self.LocX = jsobj['LocX']
	self.LocY = jsobj['LocY']
	self.Profile = jsobj['Profile']
	self.CreateDate = jsobj['CreateDate']
	self.LogoutDate = jsobj['LogoutDate']
	self.BlockDate = jsobj['BlockDate']
	self.GuildName = jsobj['GuildName']
	self.GuildID = jsobj['GuildID']
	self.GuildRank = jsobj['GuildRank']
	self.FightNum = jsobj['FightNum']
	self.FightDate = jsobj['FightDate']
	self.FightTicket = jsobj['FightTicket']
	self.QuestNum = jsobj['QuestNum']
	self.QuestID = jsobj['QuestID']
	self.QuestCount = jsobj['QuestCount']
	self.QuestRewType = jsobj['QuestRewType']
	self.QuestRewAmmount = jsobj['QuestRewAmmount']
	self.QuestCompleted = jsobj['QuestCompleted']
	self.EventID = jsobj['EventID']
	self.HP = jsobj['HP']
	self.MP = jsobj['MP']
	self.SP = jsobj['SP']
	self.PK = jsobj['PK']
	self.RewardGold = jsobj['RewardGold']
	self.State = jsobj['State']
	    
