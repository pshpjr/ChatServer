#include "stdafx.h"
#include "Player.h"


#include "CMap.h"
#include "CSerializeBuffer.h"

HashMap<SessionID, Player*> gplayers;
list<connection> loginWait;
