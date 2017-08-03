pico-8 cartridge // http://www.pico-8.com
version 8
__lua__
--           lemonhunter
--  the quest for the golden lemon
--------------------------------

--[[ license stuff
lemonhunter is released under cc4-by-nc-sa license
https://creativecommons.org/licenses/by-nc-sa/4.0/
you are free to release remixes or use parts of the code for your own non-commercial projects

death, tree and big mushroom based on art by sebastiaan van hijfte
http://i.imgur.com/rfkvq.png cc by-nc 3.0
]]

-- set to false to disable screen shake effect
screen_shake=true

-- to switch between throwing bombs and just laying them down search for
-- 'throw bomb' (line 1142)

-- objects
base_hitbox={xoffset=0,yoffset=0,w=8,h=8}
object = {
  sprite=1,
  x=0,
  y=0,
  dx=0,
  dy=0,
  life=1,
  bounciness=.5,
  lastgrounded=0,
  hitbox=base_hitbox,
  type="unset",
  bounce=true,

  move=function(self)
    move(self,false,self.bounce)
    self.dx*=.975
    if abs(self.dx)<.3 then self.dx=0 end
  end,
  damage=function(self,n,type)
    self.life-=n
  end,
  die=function(self)
    del(objects,self)
  end,
  draw=function(self)
    spr(self.sprite,self.x,self.y,1,1,self.flip)
  end,
}

function object:new(o)
	--o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o
end

bomb = object:new({
  sprite=33,
  type="bomb",
  --hitbox={xoffset=0,yoffset=2,w=5,h=6},
  dmg=3,
  radius=15,
  thrown=true,
})

red_bomb = bomb:new({
  sprite=34,
  dmg=5,
  radius=23,
})

box = object:new({
  sprite=117,
  pushable=true,
  solid=true,
  bounciness=.1,
  hitbox={xoffset=1,yoffset=2,w=6,h=6},
  damage=function(self,n,type)
    if type=="bomb" then
      del(objects,self)
      spawn_powerup(self.x,self.y)
    end
  end,
})

door = object:new({
  sprite=116,
  type="door",
})

item_plant = object:new({
  sprite=118,
  type="chest",
  damage=function(self,n)
    if n>=2 then
      del(objects,self)
      blood_anim(self.x,self.y,true)
      spawn_powerup(self.x,self.y)
    end
  end,
})

powerup = object:new({
  type="powerup",
})

gold_powerup = powerup:new({
  sprite=36,
  action="score",
  value="50",
  --hitbox={xoffset=1,yoffset=4,w=4,h=4}
})

health_powerup = powerup:new({
  sprite=32,
  value=1,
  action="health",
})

bomb_powerup = powerup:new({
  sprite=33,
  value=2,
  action="bombs",
})

skull_coin_powerup = powerup:new({
  sprite=39,
  action="skull_coin",
})

blood = object:new({
  bounce=false,
})

monster = object:new({
  sprites={base=1,walk_anim=2,walk1=1,walk2=1},
  dmg=1,
  invincible=0,
  type="monster",

  ai=function(self)
    self:move()
  end,
  move=function(self)
    move(self,true,self.thrown)
  end,
  damage=function(self,n)
    if self.invincible==0 then
      action_text("-"..n,self.x+6,self.y,8)
      self.life-=n
      if self.life>0 then
        sfx(1)
        self:bleed()
        self.invincible,self.thrown,self.dy,self.dx=16,true,-2.5,-sgn(p1.x-self.x)*(.5+n/2)
      else
        self:die()
      end
    end
  end,
  bleed=function(self)
    blood_anim(self.x,self.y)
  end,
  die=function(self)
    self:bleed()
    sfx(1)
    del(monsters,self)
    if rnd()<.2 then
      add(objects,gold_powerup:new({x=self.x,y=self.y,value=flr(rnd(50))+1}))
    end
  end,
})

ogre = monster:new({
  sprites={base=9,walk_anim=2,walk1=9,walk2=10},
  life=3,
  flip=true,

  ai=function(self)
    if not text_pause then
      self.flip=false
      self.dx=1
      if self.dy==0 and on_object(self,"door") then self:die() end
      self:move()
    end
  end,
  move=function(self)
    move(self,false,self.thrown)
  end,
  die=function(self)
    del(monsters,self)
  end
})

boss = ogre:new({
  life=15,
  flip=true,
  speed=2,
  sprite=11,
  sprites={base=11,walk_anim=2,walk1=11,walk2=12},
  ai_jump=0,
  ai=function(self)
    local self_x,self_y,self_dx,self_flip,p1_x=self.x,self.y,self.dx,self.flip,p1.x
    ai_timer+=1
    if not self.thrown then
      if ai_timer-self.ai_jump<20 and ai_timer>self.ai_jump and not self.isgrounded then
        self_dx=self_flip and 0xfffe or 2
      end
      if not (self.ai_throw_attack or self.ai_jump_attack) and self.isgrounded then
        if jumpable_step(self,self_flip) then
          self.ai_jump,self.dy,self_dx,ai_timer=30,-4,self_flip and 0xfffd or 3,30
        elseif frame%90==0 and rnd()<.50 and abs(p1_x-self_x)<48 then
          self.ai_throw_attack,ai_timer=true,0
        elseif frame%40==0 and rnd()<.42 and abs(p1_x-self_x)<32 then
          self.ai_jump_attack,self.dy,self_flip=true,0xfffd,self_x>p1_x
          self_dx=self_flip and 0xfffc or 4
        elseif frame%300==0 and rnd()<.5 and #monsters<5 then
          self_dx=0
          action_text("\\summon!",self_x+4,self_y,8)
          spawn_demon(self_x-24,self_y)
          spawn_demon(self_x+24,self_y)
        elseif frame%60==0 then
          self_flip=self_x>p1_x
          self_dx=self_flip and -1.5 or 1.5
        end
      end
      if self.ai_jump_attack and self.isgrounded then
        self.ai_jump_attack,self_dx=false,sgn(self_dx)*1.5
      elseif self.ai_throw_attack then
        if ai_timer==0 then
          self_flip=self_x<p1_x
          self_dx=self_flip and 0xfffe or 2
        elseif ai_timer<40 and self_dx==0 then
          ai_timer=39
        elseif ai_timer==40 then
          self_dx,self_flip=0,self_x>p1_x
        elseif ai_timer==50 then
          add(monsters,fireball:new({x=self_x-2,y=self_y-2,dx=self_flip and 0xfffd or 3,dy=-1.6,flip=self_flip}))
        elseif ai_timer==60 then
          self.ai_throw_attack,self_flip=false,self_x>p1_x
          self_dx=self_flip and 0xffff or 1
        end
      end
    end
    self.dx,self.flip=self_dx,self_flip
    move(self,false)
    if self.thrown then
      ai_timer=0
      if self.dy==0 then
        self.thrown=false
        self.flip=self.x>p1_x
        reset_mon_speed(self)
      end
    end
  end,
  die=function(self)
    sfx(11)
    bomb_particles(24,self.x,self.y)
    blood_anim(self.x,self.y)
    --for i=1,30 do
    --  blood_anim(i*8,48+flr(rnd(3)-1)*3)
    --end
    add(objects,skull_coin_powerup:new({x=self.x,y=self.y}))
    del(monsters,self)
    for i=1,8 do
      mset(22,4+i,100)
    end
    mset(22,6,101)
    add(actions,cocreate(function() pause_game(60) end))
  end
})

birbnana = monster:new({
  sprite=1,
  sprites={base=1,walk_anim=2,walk1=1,walk2=2},
  hitbox={xoffset=0,yoffset=0,w=7,h=8},
  dx=.6,
  ai=function(self)
    if level>1 then --don't jump in level 1
      if self.isgrounded then
        if line_of_sight(self,8,16) then
          self.dy=-2.6
        end
      end
    end
    if not self.isgrounded then
      self.dx=1.5*sgn(self.dx)
    else
      self.dx=.6*sgn(self.dx)
    end
    move(self,true)
  end
})

little_devil = monster:new({
  sprite=5,
  sprites={base=5,walk_anim=2,walk1=5,walk2=6},
  dx=.3,
  speed=.3,
  life=2,
  turn_delay=15,
  ai=function(self)
    local run=false
    if not self.thrown then
      if line_of_sight(self,12,64) then
        run=true
      end
      if run and abs(self.dx)<2.2 then
        self.dx+=sgn(self.dx)*.075
      elseif not run and abs(self.dx)>.3 then
        self.dx-=sgn(self.dx)*.05
        if abs(self.dx)<.3 then self.dx=sgn(self.dx)*.3 end
      end
    end
    move(self,not run)
    if self.dx==0 then
      turn_around(self)
      reset_mon_speed(self)
    end
    if self.dy==0 and self.thrown then
      self.thrown,self.flip=false,self.x-p1.x>0
      reset_mon_speed(self)
    end
  end,
})

bloodless_monster = monster:new({
  bleed=function(self) end,
})

rope = bloodless_monster:new({
  sprites={base=13,walk_anim=2,walk1=13,walk2=14},
  hitbox={xoffset=1,yoffset=6,w=6,h=2},
  dx=.1,
  dmg=0,
})

spikes = bloodless_monster:new({
  sprite=46,
  sprites={base=46,walk_anim=0},
  hitbox={xoffset=0,yoffset=3,w=7,h=5},
  dmg=2,
  dx=0,
  life=9,
  type="spikes",
  damage=function(self,n)
    if self.invincible==0 and n>=3 then
      self.life-=n
      if self.life>0 then
        self.invincible,self.thrown=16,true
      else
        self:die()
      end
    end
  end,
  die=function(self) del(monsters,self) end
})

fireball = bloodless_monster:new({
  sprite=47,
  sprites={base=47,walk_anim=0},
  hitbox={xoffset=0,yoffset=3,w=6,h=3},
  ai=function(self)
    self.dy-=0.14 --g/1.5
    move(self)
    if self.dy==0 then
      self.dx=0
      add(actions,cocreate(function() delayed_death(self,30) end))
    end
  end,
  die=function(self) del(monsters,self) end
})

p1=ogre:new({
  sprite=16,
  --sprites={base=16,walk1=17,walk2=18,ladder1=19,ladder2=20},
  hitbox={xoffset=0,yoffset=0,w=7,h=8},
  speed=2,

  damage=function(self,n)
    if not self.hidden and self.invincible==0 then
      sfx(6)
      action_text("-"..n,self.x+6,self.y,8)
      self.life-=n
      self:bleed()
      if self.life>0 then
        self.invincible=45
      else
        self:die()
      end
    end
  end,
  die=function(self)
    sfx(6)
    add(actions,cocreate(function() delayed_death(p1,60) end))
  end,
})

particle=object:new({
  col=6,
  draw=function(self)
    --should be split into update&draw, but i ran out of tokens...
    self.dx*=.95
    self.dy*=.95
    self.x+=self.dx
    self.y+=self.dy
    self.size-=.09
    if self.size<=0 then self:die() end
    circfill(self.x,self.y,self.size,self.col)
  end
})

-- globals
--g=0.42
frame,timer,shake,ai_timer,btn_,stop_input,cutscene_timer,jump_start,screen,paused,text_pause,monsters,objects,actions,particles,rooms,cam_x,cam_y,camera_y_offset,level,room_w,room_h,mon_sprites,obj_sprites = 0,0,0,0,0,true,0,0,nil,true,false,{},{},{},{},{},0,0,0,1,496,504,{birbnana,little_devil,spikes,rope,ogre,boss},{door,box,item_plant,gold_powerup,health_powerup}

function _init()
  --cls()
  load_rooms()
  --reset_stats()
  title_screen()
  --generate_level()
  --transition_level()
  --boss_level()
  --ending()
  --credits()
  jump_button,attack_button,button_set=4,5,false
end

function reset_stats()
  p1.life,p1.dx,p1.dy,p1.hidden,frame,timer,level,score,bombs,super_bombs,double_jump,weapon,fall_damage,skull_coin,powerups,stop_input,cutscene_timer,stop_time=3,0,0,false,0,0,1,0,3,false,false,26,true,false,{},false,0,false
end

function title_screen()
  generate_level()
  screen="title"
end

function transition_level()
  screen,paused,p1.flip,room_w,room_h="transition",false,false,128,128
  set_room(0,0,rooms.transition)
  if level==2 then
    text_box("\\you seek our treasure?","\\turn back, it won't be","so easy from here on.")
  elseif level==4 then
    text_box("\\you will never find","the secret","lemon treasure!")
  else
    text_box("\\d\\e\\a\\t\\h guards","the lemon treasure.")
  end
end

function boss_level()
  clear_objects()
  screen,paused,p1.flip,room_w,room_h="boss",false,false,248,128
  set_room(0,0,rooms.boss,30,15)
  text_box("\\you should not","have come here.")
end

function ending()
  clear_objects()
  p1.dx,p1.dy,stop_time,screen,paused,stop_input,cam_x,p1.flip,room_w,room_h=0,0,true,"ending",false,true,8,false,128,128
  set_room(0,0,rooms.ending,16,15)
end

function credits()
  camera()
  screen,paused,frame="credits",true,0
end

function _draw()
  cls()
  if not paused then draw_level() end
  if screen=="title" then
    draw_title()
  elseif screen=="ending" then
    draw_ending()
  elseif screen=="credits" then
    draw_credits()
  end
  high_load=stat(1)>=.93
  over_load=stat(1)>1
  --rectfill(cam_x,cam_y+122,cam_x+127,cam_y+127,0)
  --print("load "..stat(1(.." / "..flr(stat(0)/10.24).."% "..#monsters..","..#objects,cam_x+1,cam_y+123,stat(1)<=1 and 12 or 8)
end

function draw_sprite(obj)
  obj:draw()
end

function draw_level()
  if screen!="title" and screen!="ending" then
    cam_x,cam_y=p1.x-64,p1.y-64
    if p1.flip then cam_x+=1 end
    if cam_x<0 then cam_x=0
    elseif cam_x>room_w-128 then cam_x=room_w-128 end
    if cam_y<0 then cam_y=0
    elseif cam_y>room_h-128 then cam_y=room_h-128 end
    if shake>0 then
      cam_x+=rnd(3)-1
      cam_y+=rnd(3)-1
    end
  end
  camera(cam_x,cam_y)
  map_x,map_y=flr(cam_x/8)-1,flr(cam_y/8)-1
  map(map_x,map_y,map_x*8,map_y*8,18,18)
  foreach(objects, draw_sprite)
  palt(0,false)
  palt(12,true)
  foreach(monsters, draw_sprite)
  palt()
  if screen!="title" then
    info_bar()
    if not p1.hidden then
      if p1.invincible==0 or frame%2==0 then
        p1:draw()
      end
      if up_icon(p1) then
        spr(62,p1.x+5,p1.y-3,1,1)
      end
    end
    for c in all(actions) do
      if costatus(c) then
        coresume(c)
      else
        del(actions,c)
      end
    end
  end
  foreach(particles, draw_sprite)
end

function draw_title()
  if frame>10 and (btn(jump_button) or btn(attack_button)) then
    if btn(5) and not button_set then
      jump_button=5
      attack_button=4
    end
    button_set=true
    reset_stats() generate_level() return
  end
  btn_,cam_y=127,32
  rectfill(cam_x+31,65,cam_x+85,71,0)
  spr(48,cam_x+32,64)
  print("lemonhunter",cam_x+42,66,10)
  rectfill(cam_x-1,148,cam_x+127,159,0)
  sspr(64,16,16,16,cam_x+2,141)
  if frame%840<240 then
    if not button_set then
      shorttext("select jump key",cam_x+38,152)
    else
      shorttext("press \x8e /\x97  to start",cam_x+30,152)
    end
  elseif frame%840<480 then
    if jump_button == 4 then
      shorttext("\x8e jump \x97 attack \x83 \x97 bomb",cam_x+22,152)
    else
      shorttext("\x97 jump \x8e attack \x83 \x8e bomb",cam_x+22,152)
    end
  elseif frame%840<720 then
    shorttext("1000  = 1",cam_x+52,152)
    spr(36,cam_x+68,149)
    spr(32,cam_x+89,149)
  else
    shorttext("2017 - blizzz",cam_x+42,152,5)
  end
  if title_left then
    cam_x-=.5
    if cam_x<=0 then title_left=false end
  else
    cam_x+=.5
    if cam_x>=368 then title_left=true end
  end
end

function draw_ending()
  if not text_pause then
    cutscene_timer+=1
    if cutscene_timer==10 then
      text_box("\\d\\e\\a\\t\\h","\\i lost my skull coin.")
    end
    if skull_coin then
      if cutscene_timer==40 then
        text_box("\\d\\e\\a\\t\\h","\\give me my coin","and you may take","a lemon!")
      elseif cutscene_timer==90 then
        del(powerups,skull_coin_powerup.sprite)
        add(objects,skull_coin_powerup:new({x=p1.x,y=p1.y,dx=2,dy=0xfffc}))
      elseif cutscene_timer>90 and cutscene_timer<200 then
        objects[1]:move()
      elseif cutscene_timer==200 then
        p1.dx=1
      elseif cutscene_timer==320 then
        p1.dx=0
        action_text("\x87",112,93,8)
      elseif cutscene_timer==350 then
        local speedrun,timebonus=300-timer,0
        if speedrun>0 then
          if speedrun>240 then
            timebonus=(speedrun-240)*250+10200
          elseif speedrun>180 then
            timebonus=(speedrun-180)*100+4200
          elseif speedrun>120 then
            timebonus=(speedrun-120)*50+1200
          elseif speedrun>60 then
            timebonus=(speedrun-60)*15+300
          else
            timebonus=speedrun*5
          end
          text_box("\\you won!","","\\speedrun \\bonus: +"..timebonus,"\\score: "..score+timebonus)
        else
          text_box("\\you won!","","\\score: "..score)
        end
      elseif cutscene_timer==360 then credits()
      end
    else
      if cutscene_timer==60 then
        text_box("\\d\\e\\a\\t\\h","\\you don't have it...","\\go back.")
      elseif cutscene_timer==70 then
        p1.flip=true
        p1.dx=-.5
      elseif cutscene_timer==128 then
        action_text("meow",98,93,2)
      elseif cutscene_timer==230 then
        credits()
      end
    end
    p1:move()
  end
end

function draw_credits()
  if frame>150 and (btn(jump_button) or btn(attack_button)) then title_screen() return end
  frame+=1
  local credit_text={
    "\\lemonhunter",
    "",
    "a game by",
    "blizzz",
    "",
    "win32 player by",
    "old\\ganon",
    "",
    "\\special \\thanks",
    "\\lemon, \\h'orvuhst, old\\ganon",
    "\\natori (like,share,subscribe)",
    "\\nab, \\pan, \\wash, \\seb, \\tom",
    "\\snuki (who liked this game)",
    "\\michelle, \\baka, \\twig",
    "",
    "\\thanks for playing!",
  }
  for i=1,#credit_text do
    shorttext(credit_text[i],hcenter(credit_text[i]),128+8*i-frame/4)
    if i==#credit_text and 128+8*i-frame/4==0xfff8 then title_screen() end
  end
end

function info_bar()
  rectfill(cam_x,cam_y,cam_x+127,cam_y+7,0)
  spr(32,cam_x+1,cam_y-2)
  print(p1.life,cam_x+8,cam_y+1,5)
  spr(super_bombs and 34 or 33,cam_x+17,cam_y-2)
  print(bombs,cam_x+24,cam_y+1,5)
  spr(weapon,cam_x+33,weapon!=28 and cam_y-2 or cam_y)
  local item_x=cam_x+47
  for powerup in all(powerups) do
    spr(powerup,item_x,cam_y-1)
    item_x+=10
  end
  spr(35,cam_x+77,cam_y-2)
  spr(0,cam_x+85,cam_y+2)
  print(score,cam_x+90,cam_y+1,5)
  t_min,t_sec=flr(timer/60),timer%60
  print(t_min..":"..(t_sec<10 and "0" or "")..t_sec,cam_x+(t_min>9 and 108 or 112),cam_y+1,5)
end

function hcenter(s)
  c=64-flr((#s*4)/2)
  for i=1,#s do
    if sub(s,i,i)=="\\" then
      c+=2
    end
  end
  return c
end

function move(obj,patrol,bounce)
  local patrol,bounce,w,h,x_gap_l=patrol or false,bounce or false,obj.hitbox.w,obj.hitbox.h,obj.hitbox.xoffset
  local x_gap_r=8-(x_gap_l+w)
  if obj.flip then
    x_gap_l=x_gap_r
    x_gap_r=obj.hitbox.xoffset
  end

  obj.x+=obj.dx/2

  --hit side walls
  local xoffset,hit_object,object_x=x_gap_l,false
  if obj.dx>0 then xoffset=x_gap_l+w-1 end

  if abs(obj.dx)!=0 then
    local ht,hb=mget((obj.x+xoffset)/8,(obj.y+2)/8),mget((obj.x+xoffset)/8,(obj.y+7)/8)
    if obj!=p1 then
      for o in all(objects) do --check boxes
        if o.solid and o!=obj then
          if obj.x-x_gap_r>=o.x-7 and obj.x+x_gap_l<=o.x+7 and
              obj.y+5>=o.y and obj.y<=o.y and
              (obj.dx>0 and obj.x<o.x or obj.dx<0 and obj.x>o.x) then
            hit_object=true
            object_x=o.x
          end
        end
      end
    end

    if fget(ht,2) or fget(hb,2) or hit_object then
      obj.x=flr((obj.x)/8)*8
      if obj.dx<0 then
        obj.x+=8-x_gap_l
        if hit_object then obj.x=object_x+7-x_gap_l end
      else
        obj.x+=x_gap_r
        if hit_object then obj.x=object_x-7+x_gap_r end
      end
      if patrol then
        turn_around(obj)
      else
        if bounce and abs(obj.dx)>=.5 then
          obj.dx*=0xffff*obj.bounciness
          obj.flip=not obj.flip
        else
          obj.dx=0
        end
      end
    end

    if hit_object and fget(mget((obj.x+(object_x>obj.x and 0 or 7))/8,(obj.y+7)/8),2) then --crushed
      if obj.type=="bomb" then
        obj.x=object_x+(object_x>obj.x and 0xfffc or 3)
      else
        obj:die()
      end
      return
    end
  end

  --gravity
  if not obj.onladder then
    obj.dy+=0.21
    if obj.dy>8.5 then obj.dy=8.5 end
  end

  obj.y+=obj.dy/2

  if obj.onladder and obj.dy<0 then
    if not on_ladder(obj) then
      obj.y=flr((obj.y)/8)*8+3
    end
  end

  --check floor
  obj.isgrounded=false
  local ground_offset=obj.hitbox.yoffset+obj.hitbox.h
  local vl,vr,hit_object,object=mget((obj.x+x_gap_l +1)/8,(obj.y+ground_offset)/8),mget((obj.x+x_gap_l+w-2)/8,(obj.y+ground_offset)/8),false

  if obj.dy>=0 then
    for o in all(objects) do --check boxes
      if o.solid and o!=obj then
        if obj.x-x_gap_r>=o.x-5 and obj.x+x_gap_l<=o.x+5 then
          if obj.y+7>=o.y and obj.y<=o.y then
            hit_object=true
            object=o
          end
        end
      end
    end

    if fget(vl,2) or fget(vr,2) or hit_object
      or not obj.onladder and (fget(vl,4) or fget(vr,4)) and (obj.dy>4 or obj.y%8<3) then
      if obj==p1 and obj.dy==8.5 then
        jump_particles()
        if p1.invincible==0 and fall_damage then
          p1:damage(1)
          bounce,p1.thrown,p1.invincible=true,true,60
        end
      end
      obj.y = flr((obj.y)/8)*8+8-ground_offset
      if object!=nil then
        obj.y=object.y-8+object.hitbox.yoffset
      end
      if bounce and abs(obj.dy)>=.5 then
        sfx(4)
        obj.dy*=0xffff*obj.bounciness
        obj.dx*=.8
      else
        obj.dy,obj.isgrounded=0,true
      end
    end

    if patrol and obj.dx!=0 and obj.isgrounded then
      if obj.dx<0 and not (fget(vl,2) or fget(vl,4)) or obj.dx>0 and not (fget(vr,2) or fget(vr,4)) then
        obj.dy=0
        turn_around(obj)
      end
    end
  end

  obj.lastgrounded=(obj.isgrounded or obj.onladder) and 0 or obj.lastgrounded+1

  --hit ceiling
  if obj.dy<=0 then
    vl,vr=mget((obj.x+x_gap_l +1)/8,(obj.y+2)/8),mget((obj.x+x_gap_l+w-2)/8,(obj.y+2)/8)
    if fget(vl,2) or fget(vr,2) then
      obj.y,obj.dy=flr((obj.y+7)/8)*8-2,0
    end
  end
  local screen_top=screen=="level" and 0 or 8
  if obj.y<screen_top then
    obj.y=screen_top
    obj.dy*=.90
  end
end

function turn_around(obj)
  obj.flip=not obj.flip
  obj.dx*=0xffff
end

function reset_mon_speed(mon)
  mon.dx=mon.flip and -mon.speed or mon.speed
end

function jumpable_step(mon,flip)
  local xoffset=16
  if flip then xoffset=0xfff8 end
  local ht,hb=mget((mon.x+xoffset)/8,(mon.y-7)/8),mget((mon.x+xoffset)/8,(mon.y+1)/8)
  return fget(hb,2) and not fget(ht,2)
end

function collision(x,y,hitbox,flip,group)
  local hits,hitbox={},hitbox or base_hitbox
  if group==nil then --all
    hits=collision(x,y,hitbox,flip,monsters)
    for hit in all(collision(x,y,hitbox,flip,objects)) do
      add(hits,hit)
    end
  else
    local x_left=x+hitbox.xoffset
    local x_right=x_left+hitbox.w
    if flip then
      local xoffset=8-hitbox.xoffset-hitbox.w
      x_left,x_right=x+xoffset,x+xoffset+hitbox.w
    end
    local y_top=y+hitbox.yoffset
    local y_bottom=y_top+hitbox.h

    for obj in all(group) do
      if abs(x-obj.x)<8 and abs(y-obj.y)<8 then
        local obj_x_left=obj.x+obj.hitbox.xoffset
        local obj_x_right=obj_x_left+obj.hitbox.w
        if obj.flip then
          xoffset=8-obj.hitbox.xoffset-obj.hitbox.w
          obj_x_left,obj_x_right=obj.x+xoffset,obj.x+xoffset+obj.hitbox.w
        end
        local obj_y_top=obj.y+obj.hitbox.yoffset
        local obj_y_bottom=obj_y_top+obj.hitbox.h
        if x_right-1>obj_x_left and x_left<obj_x_right then
          if y_bottom>obj_y_top and y_top<obj_y_bottom then
            add(hits,obj)
          end
        end
      end
    end
  end
  return hits
end

function box_crush()
 for obj in all(objects) do
   if obj.solid and obj.dy>0 then
     mons = collision(obj.x,obj.y,obj.hitbox,obj.flip,monsters)
     for mon in all(mons) do
       if mon.type!="spikes" then
         mon:die()
       end
     end
   end
 end
end

function dist(obj,x,y)
  local xoffset,yoffset=obj.x+7<=x and 7 or 0,obj.y+7<=y and 7 or 0
  local x_dist=x-(obj.x+xoffset)
  if x>=obj.x and x<=obj.x+7 then
    x_dist=0
  end
  local y_dist=y-(obj.y+yoffset)
  if y>=obj.y and y<=obj.y+7 then
    y_dist=0
  end
  local dsq=(x_dist/1000)^2+(y_dist/1000)^2
  if dsq>32 then
    return 32767
  elseif dsq>0 then
    return sqrt(dsq)*1000
  else
    return 0
  end
end

function on_ladder(char)
  x,y=char.x,char.y
  spr1,spr2,spr3,spr4=mget((x+3)/8,y/8),mget((x+5)/8,y/8),mget((x+3)/8,(y+5)/8),mget((x+5)/8,(y+5)/8)
  return (fget(spr1,3) or fget(spr2,3) or fget(spr3,3) or fget(spr4,3))
end

function on_object(char,type1,type2)
  local objs,hit=collision(char.x,char.y,char.hitbox,char.flip),false
  for obj in all(objs) do
    if obj.type==type1 or obj.type==type2 then
      hit=obj
      break
    end
  end
  return hit
end

function up_icon(char)
  return on_object(char,"door","chest")
end

function line_of_sight(char,dy,dx)
  local x=char.x
  local dist=x-p1.x
  if abs(char.y-p1.y)>dy or abs(dist)>dx or char.flip and dist<0 or not char.flip and dist>0 then
    return false
  else
    local los=true
    for i=1,flr(abs(p1.x-x)/8) do
      local x=char.flip and (x+2)/8-i or (x+5)/8+i
      if fget(mget(x,char.y/8),2) then los=false end
    end
    return los
  end
end

function get_random_item(obj)
  local luck=rnd()
  if luck<.05 then
    if not add_powerup("double_jump") then get_random_item() end
  elseif luck<.15 then
    upgrade_weapon()
  elseif luck<.25 then
    if not add_powerup("super_bombs") then get_random_item() end
  elseif luck<.35 then
    if not add_powerup("fall_damage") then get_random_item() end
  elseif luck<.70 then
    add_powerup("bombs",2)
  elseif luck<.85 then
    add_powerup("health")
  else
    add_powerup("score",flr(rnd(101))+50)
  end
  if obj then obj:die() end
end

function upgrade_weapon()
  local x_=p1.x+2
  if weapon==26 then
    if rnd()<.3 then
      weapon=15
      action_text("\\magical \\twig",x_,p1.y)
    else
      weapon=27
      action_text("\\spear",x_,p1.y)
    end
    sfx(13)
  elseif weapon==27 and rnd()>.5 then
    weapon=28
    action_text("\\flame \\sword",x_,p1.y)
    sfx(13)
  else
    get_random_item()
  end
end

powerup_objs_lv1={gold_powerup,bomb_powerup,health_powerup}
function spawn_powerup(x,y,l)
  local item,r=1,rnd()
  if r<.21 then
    item=3
  elseif r<.42 then
    item=2
  end
  add(objects,powerup_objs_lv1[item]:new({x=x,y=y}))
end

function add_powerup(action,n,x,y)
  local x,y=x or p1.x+2,y or p1.y
  if action=="score" then
    if flr(score/1000)!=flr((score+n)/1000) then
      p1.life+=1
      action_text("life +1",x,y,11,30)
      sfx(14)
    else
      sfx(2)
    end
    score+=n
    action_text(n.." gold",x,y,9)
  elseif action=="health" then
    p1.life+=1
    action_text("life +1",x,y,11)
    sfx(14)
  elseif action=="bombs" then
    bombs+=n
    action_text("bombs +"..n,x,y)
    sfx(14)
  elseif action=="double_jump" and not double_jump then
    double_jump=true
    add(powerups,38)
    action_text("\\birb \\feather",x,y)
    sfx(13)
  elseif action=="fall_damage" and fall_damage then
    fall_damage=false
    add(powerups,37)
    action_text("\\cat's \\paws",x,y)
    sfx(13)
  elseif action=="super_bombs" and not super_bombs then
    super_bombs=true
    action_text("\\crimson \\coating",x,y)
    sfx(13)
  elseif action=="skull_coin" and not skull_coin then
    skull_coin=true
    add(powerups,39)
    action_text("?",x,y)
    sfx(12)
  else
    return false
  end
  return true
end

function _update60()
  if shake>0 then shake-=1 end
  if paused or over_load and frame<60 then return end

  frame+=1
  --if frame==32767 then frame=1 end
  if frame%60==0 and not stop_time then timer+=1 end
  if text_pause then return end

  if p1.invincible>0 and not p1.thrown then p1.invincible-=1 end
  for mon in all(monsters) do
    if mon.invincible>0 then mon.invincible-=1 end
  end

  if not p1.hidden and not stop_input then
    --walk
    if not p1.thrown then
      if btn(0) then --left
        if p1.dx>-p1.speed then p1.dx-=p1.speed/8 end
        if p1.dx<-p1.speed then p1.dx=-p1.speed end
        if not p1.flip then p1.x=flr(p1.x) p1.x-=1 end --for hitbox reasons
        p1.flip=true
      end
      if btn(1) then --right
        if p1.dx<p1.speed then p1.dx+=p1.speed/8 end
        if p1.dx>p1.speed then p1.dx=p1.speed end
        if p1.flip then p1.x+=1 end --for hitbox reasons
        p1.flip=false
      end
      if not (btn(0) or btn(1)) then
        p1.dx*=.8
        if abs(p1.dx)<.2 then p1.dx=0 end
      end
      if p1.onladder then p1.dx*=.7 end
      if btn(2) then --ladder up
        if on_ladder(p1) then
          if not p1.onladder then p1.sprite=19 end
          p1.onladder,p1.dy=true,-.5*p1.speed
        end
      end
      if btnp_(2) then
        local chest_check=on_object(p1,"chest")
        if chest_check then --chest
          get_random_item(chest_check)
        elseif on_object(p1,"door") then --door
          if screen=="boss" then
            ending()
          elseif level<7 then
            next_floor()
          else
            boss_level()
          end
        end
      end
      if btn(3) then --ladder down
        if on_ladder(p1) then
          if not p1.onladder then p1.sprite=19 end
          p1.onladder,p1.dy=true,.5*p1.speed
        end
      end
      if p1.onladder then
        if not btn(2) and not btn(3) then p1.dy=0 end
        if not on_ladder(p1) then p1.onladder=false end
      end
      --jump
      if (btnp_(jump_button) and (p1.lastgrounded<=2 or p1.onladder or (double_jump and p1.lastgrounded<200))) then
          p1.onladder,p1.dy,jump_start=false,-4.2,frame
          sfx(5)
          jump_particles()
          if p1.lastgrounded>2 then
            p1.lastgrounded+=200
          end
      elseif frame-jump_start==5 and not btn(jump_button) then --smol jump
          p1.dy+=1.6
      end
    else --throw animation
      p1.dx-=sgn(p1.dx)*0.021 --g*.1
      if p1.dy==0 then p1.thrown=false end
      p1.onladder=false
    end

    p1:move()

    local objs = collision(p1.x,p1.y,p1.hitbox,p1.flip,objects)
    if #objs>0 then
      for obj in all(objs) do
        if obj.type=="powerup" then --pickup powerup
          add_powerup(obj.action,obj.value,obj.x+2,obj.y)
          del(objects,obj)
        elseif obj.pushable and p1.dx!=0 then --push boxes
          if sgn(obj.x-p1.x)==sgn(p1.dx) and
              abs(obj.x-p1.x)<5 and
              obj.y<=p1.y+3 and obj.y>=p1.y-4 then
            obj.dx=p1.dx/2
            obj:move(false)
            obj.dx=0
            for o in all(collision(obj.x,obj.y,base_hitbox,false,monsters)) do
              o.x=obj.x+(p1.dx<0 and 0xfffa or 6)
            end
            p1.x=obj.x+(p1.dx<0 and 5 or 0xfffb)
          end
        end
      end
    end

    if (btnp_(attack_button) and p1.attacking==false) then
      if btn(3) and bombs>0 and not p1.onladder then --throw bomb
        --enable the next two lines and disable the third to drop bombs instead
        --add(actions,cocreate(function() set_bomb(p1.x+(p1.flip and 1 or 0xffff),p1.y,0,0) end))
        --sfx(4)
        add(actions,cocreate(function() set_bomb(p1.x+(p1.flip and 0 or 4),p1.y,p1.dx+(p1.flip and 0xffff or 1),p1.dy-2.5) end))
      else --attack
        p1.attacking=true
        add(actions,cocreate(attack_anim))
      end
    end

    if p1.invincible==0 then --monster collision detection
      local mons = collision(p1.x,p1.y,p1.hitbox,p1.flip,monsters)
      if #mons>0 then
        local pick_mon={1,0}
        for i=1,#mons do --don't take damage from monsters we just hit
          if mons[i].invincible==0 and mons[i].dmg>pick_mon[2] then pick_mon={i,mons[i].dmg} end
        end
        if pick_mon[2]>0 then
          p1:damage(pick_mon[2])
          p1.thrown,p1.dx,p1.dy=true,1.5,-3.5
          if mons[pick_mon[1]].x>p1.x then
            p1.dx*=0xffff
          end
        end
      end
    end
  end

  for obj in all(objects) do
    if abs(obj.x-p1.x)<96 and abs(obj.y-p1.y)<96 and screen!="title" then
      obj:move()
    end
  end
  box_crush()

  for mon in all(monsters) do
    if abs(mon.x-p1.x)<128 and abs(mon.y-p1.y)<128 or screen=="title" and mon.y>=26 and mon.y<=160 and mon.x>=cam_x-8 and mon.x<=cam_x+136 then
      mon:ai()
      if mon.dy>0 and mon.type!="spikes" then --spikes
        mons=collision(mon.x,mon.y,mon.hitbox,mon.flip,monsters)
        for m in all(mons) do
          if m.type=="spikes" and m!=mon then mon:die() break end
        end
      end
    end
  end

  --animation
  if not p1.attacking then
    if p1.onladder then
      if frame%8==0 and p1.dy!=0 then
        p1.sprite=p1.sprite==19 and 20 or 19
      end
    elseif p1.dx!=0 then
      if frame%8==0 then
        p1.sprite=p1.sprite==17 and 18 or 17
      end
    end
    if p1.dx==0 and not p1.onladder then p1.sprite=16 end
  end

  if frame%12==0 then
    for mon in all(monsters) do
      if mon.dx!=0 and mon.sprites.walk_anim==2 then
        mon.sprite=mon.sprite==mon.sprites.walk1 and mon.sprites.walk2 or mon.sprites.walk1
      end
    end
  end

  btn_ = btn()
end

--load room layouts into ram
function load_rooms()
  rooms={{load_room(1,1),load_room(16,1),load_room(62,0)}, --1 start
        {load_room(31,1),load_room(46,1),load_room(1,16),load_room(31,46), --2 passage
        load_room(77,32),load_room(92,32),load_room(107,32),load_room(92,0),
        load_room(46,46),load_room(107,0),load_room(62,15),load_room(77,15)},
        {load_room(1,46),load_room(16,46)}, --3 drop
        {load_room(31,16),load_room(46,16),load_room(77,0)}, --4 goal
        {load_room(1,31),load_room(16,31),load_room(31,31),load_room(46,31), --5 optional
        load_room(16,16),load_room(62,32),load_room(92,15),load_room(107,15)}}
  rooms.transition=load_room(63,49)
  rooms.boss=load_room(78,49,30)
  rooms.ending=load_room(108,49,16)
end

--return room layout from map rom
function load_room(x,y,w,h)
  local w,h,room=w or 15,h or 15,{}
  for i=1,h do
    room[i]={}
    for j=1,w do
      room[i][j]=mget(x+j-1,y+i-1)
    end
  end
  return room
end

function clear_objects()
  monsters,objects,actions,particles,p1.attacking,shake={},{},{},{},false,0
end

--set a new floor
function next_floor()
  clear_objects()
  p1.dx=0
  level+=1
  if level%2==0 then
    sfx(8)
    transition_level()
  else
    generate_level()
  end
end

--procedurally generate a level
function generate_level()
  clear_objects()
  screen,frame,paused,room_w,room_h="level",0,false,496,504
  local goal,map=false,{{--[[0,0,0,0]]}, --0 empty, 1 start
                        {--[[0,0,0,0]]}, --2 passage, 3 drop
                        {--[[0,0,0,0]]}, --4 goal
                        {--[[0,0,0,0]]}}
  local x,y=flr(rnd(100)%4)+1,1
  map[y][x]=1
  while not goal do
    d=get_direction(x)
    if d!=0 then
      x+=d
      if map[y][x]==nil then map[y][x]=2 end
    elseif map[y][x]!=3 then --prevent multiple drops
      if y<4 then
        y+=1
        map[y][x]=3
      else
        map[y][x],goal=4,true
      end
    end
  end
  --create rooms
  for i=0,3 do
    for j=0,3 do
      place_room(j,i,map[j+1][i+1])
    end
  end
  --despawn objects to save cpu time
  while #monsters>50 do
    del(monsters,monsters[flr(rnd(100)%#monsters)+1])
  end
  while #objects>42 do
    local o=objects[flr(rnd(100)%#objects)+1]
    if not o.solid and o.type!="door" then
       del(objects,o)
     end
  end
end

function get_direction(i)
  local d=flr(rnd(100)%3)-1
  if d==0xffff and i==1 then
    d=1
  elseif d==1 and i==4 then
    d=0xffff
  end
  return d
end

function place_room(x,y,n)
  local n=n or 5
  set_room(x,y,rooms[n][flr(rnd(100)%#rooms[n])+1])
  if n==3 then --drop
    --[[for i=1,15 do
      if mget(y*15+i,x*15+1)==100 then
        local s=0
        while fget(mget(y*15+i,x*15-s),2) do
          mset(y*15+i,x*15-s,100)
          s+=1
        end
        if rnd()<.5 then s-=1 end
        mset(y*15+i,x*15-s+1,101)
        mset(y*15+i,x*15-s,100)
        mset(y*15+i,x*15-s-1,0)
      end
    end]]
    --using fixed ladder position to save tokens
    local ladder_x=y*15+3
    mset(ladder_x,x*15,101)
    mset(ladder_x,x*15-1,100)
  end
end

function set_room(x,y,room,w,h)
  local w,h=w or 15,h or 15
  for i=1,h do
    for j=1,w do
      set_tile(y*15+j,x*15+i,room[i][j])
    end
  end
end

--parse tile data and place tile or monster
function set_tile(x,y,n)
  if n==16 then
    p1.x,p1.y=x*8,y*8
    mset(x,y,0)
  else
    if n==118 and rnd()<.60 then n=36 end
    local obj=is_object(n)
    if obj then
      obj=obj:new({x=x*8,y=y*8})
      random_flip(obj)
      if obj.action=="score" then obj.value=flr(rnd(26)+25) end
      if obj.type!="chest" and obj.action!="score" or rnd()>.42 then
        add(objects,obj)
      end
      mset(x,y,0)
    else
      local mon=is_monster(n)
      if mon then
        mon=mon:new({x=x*8,y=y*8})
        if level>1 and n==1 then --indicate jumping
          mon.sprites={base=3,walk_anim=2,walk1=3,walk2=4}
        elseif n==5 and rnd()<.01 then
          mon.sprites={base=7,walk_anim=2,walk1=7,walk2=8}
        end
        mon.sprite=mon.sprites.base
        random_flip(mon)
        add(monsters,mon)
        mset(x,y,0)
      else
        if n!=0 then
          mset(x,y,n)
        else
          if rnd()>.92 then
            mset(x,y,flr(124.3+rnd(3)))
          else
            mset(x,y,0)
          end
        end
      end
    end
  end
end

--convert sprite to object
function is_object(n)
  local obj=false
  for x in all(obj_sprites) do
    if x.sprite==n then
      obj=x
    end
  end
  return obj
end

--convert sprite to monster object
function is_monster(n)
  local mon=false
  for x in all(mon_sprites) do
    if x.sprites.base==n then
      mon=x
    end
  end
  return mon
end

function random_flip(obj)
  if rnd()>=.5 then
    obj.dx*=0xffff
    obj.flip=true
  end
end

--spawn a demon
function spawn_demon(x,y)
  if x>=8 and x<=240 then
    add(actions,cocreate(function() spawn_demon_co(x,y-24) end))
  end
end

function spawn_demon_co(x,y)
  local mon,fire=little_devil:new({x=x,y=y}),115
  random_flip(mon)
  for i=1,60 do
    if i==30 then fire=99 end
    spr(fire,x,y,1,1,mon.flip)
    yield()
  end
  add(monsters,mon)
end

--spawn a bomb
function set_bomb(x,y,dx,dy)
  bombs-=1
  local bomb_obj_data,b={x=x,y=y,dx=dx,dy=dy}
  if super_bombs then b=red_bomb:new(bomb_obj_data)
  else b=bomb:new(bomb_obj_data) end
  add(objects,b)
  for i=1,120 do
    yield()
  end
  del(objects,b)
  x,y=b.x+2,b.y+4
  x_,y_=flr(x/8),flr(y/8)
  local range,r,dmg=super_bombs and 3 or 2,b.radius,b.dmg
  for i=-range,range do
    local xoffset=0
    if i<0 then xoffset=7 end
    for j=-range,range do
      local yoffset=0
      if j<0 then yoffset=7 end
      local tile_x,tile_y=x_+i,y_+j
      local tile=mget(tile_x,tile_y)
      if fget(tile,7) then
        if (x-((flr(x/8)+i)*8+xoffset))^2 + (y-((flr(y/8)+j)*8+yoffset))^2 <= r^2 then
          mset(tile_x,tile_y,0)
          if fget(mget(tile_x,tile_y-1),6) then
            mset(tile_x,tile_y-1,0)
          end
        end
      end
    end
  end
  sfx(3)
  if dist(p1,x,y)<=r then
    p1:damage(dmg)
    p1.thrown,p1.dx,p1.dy=true,4.2,-4.2
    if x>p1.x then
      p1.dx*=0xffff
    end
  end
  for mon in all(monsters) do
    if dist(mon,x,y)<=r then
      mon:damage(dmg)
    end
  end
  for obj in all(objects) do
    if dist(obj,x,y)<=r then
      obj:damage(dmg,"bomb")
    end
  end
  if screen_shake then shake=15 end
  if not high_load then
    bomb_particles(r,x,y)
  end
  for i=0,5 do
    circfill(x,y,r,10-flr(i/2))
    yield()
  end
end

function bomb_particles(r,x,y)
  for i=1,r>15 and 20 or 10 do
    p=particle:new({x=x+rnd(r)-r/2,y=y+rnd(r)-r/2,size=rnd(7),dx=rnd(1)-.5,dy=rnd(1)-.5,col=5.5+rnd(2)})
    add(particles,p)
  end
end

function jump_particles()
  if not high_load then
    jump_particle(p1.flip and p1.x+3 or p1.x+1,p1.y+6,-.42)
    jump_particle(p1.flip and p1.x+6 or p1.x+4,p1.y+6,.42)
  end
end

function jump_particle(x,y,dx)
  add(particles,particle:new({x=x,y=y,dx=dx,size=1.9}))
end

--attack with main weapon (coroutine)
function attack_anim()
  if weapon==28 then
    sfx(10)
  else
    sfx(0)
  end
  local dmg,hitbox=1,{xoffset=0,yoffset=4,w=7,h=3}
  if weapon==27 then
    dmg,hitbox=2,{xoffset=0,yoffset=3,w=7,h=5}
  elseif weapon==28 then
    dmg,hitbox=3,base_hitbox
  elseif weapon==15 then
    dmg,hitbox=999,{xoffset=0,yoffset=2,w=8,h=5}
  end
  local wpn_spr=weapon
  for i=0,14 do
    if weapon==28 and (i==2 or i==4) then wpn_spr+=1 end
    x,x2=p1.x,p1.x
    if p1.flip then
      x-=7
      if weapon!=28 then x+=i/7*3 end
      x2-=14
    else
      x+=7
      if weapon!=28 then x-=i/7*3 end
      x2+=14
    end
    p1.sprite=21
    spr(wpn_spr,x,p1.y,1,1,p1.flip)
    local hits1=collision(x,p1.y,hitbox)
    if #hits1>0 then
      for obj in all(hits1) do
        obj:damage(dmg,"attack")
        if weapon==15 and obj.type=="monster" then
          weapon=26
          if screen=="boss" then
            text_box("\\twig broke,","but fulfilled his goal.")
          else
            action_text("\\twig broke :(",p1.x+6,p1.y,6,30)
          end
        end
      end
    end
    if weapon==28 and i>=4 then
      spr(31,x2,p1.y,1,1,p1.flip)
      local hits2=collision(x2,p1.y,{xoffset=2,yoffset=0,w=4,h=5})
      if #hits2>0 then
        for obj in all(hits2) do
          local already_hit=false
          for o in all(hits1) do
            if o==obj then already_hit=true end
          end
          if not already_hit then
            obj:damage(2)
          end
        end
      end
    end
    yield()
  end
  p1.sprite,p1.attacking=16,false
end

--spawn blood effect (coroutine)
function blood_anim(x,y,green)
  add(actions,cocreate(function() blood_anim_co(x,y,green) end))
end

function blood_anim_co(x,y,green)
  local flipped,floor,sprite,dy=false,false,green and 53 or 50,0
  local blood_obj=blood:new({x=x,y=y,sprite=sprite})
  random_flip(blood_obj)
  add(objects,blood_obj)
  for i=0,12 do
    if i==8 then blood_obj.sprite=sprite+1 end
    blood_obj.dy-=0.14
    yield()
  end
  while not blood_obj.isgrounded do
    if green then
      blood_obj.dy-=0.105
      if blood_obj.dy>4 then blood_obj.dy=4 end
    end
    yield()
  end
  blood_obj.sprite=sprite+2
  for i=0,60 do
    yield()
  end
  del(objects,blood_obj)
end

function delayed_death(mon,n)
  if mon==p1 then
    p1.hidden=true
    p1:bleed()
  end
  for i=1,n do
    yield()
  end
  if mon==p1 then
    --sfx(17)
    text_box("\\you died.","","\\score: "..score,"",true)
  else
    mon:die()
  end
end

--text box coroutine
function text_box(line1,line2,line3,line4,death)
  text_pause=true
  add(actions,cocreate(function() text_box_co(line1,line2,line3,line4,death) end))
end

function text_box_co(line1,line2,line3,line4,death)
  if death then stop_time=true end
  for i=1,240 do
    if btnp_(jump_button) or btnp_(attack_button) then break end
    local text_x=cam_x+20
    rectfill(cam_x+16,cam_y+30,cam_x+112,cam_y+66,1)
    shorttext(line1,text_x,cam_y+34)
    shorttext(line2,text_x,cam_y+42)
    shorttext(line3,text_x,cam_y+50)
    shorttext(line4,text_x,cam_y+58)
    yield()
  end
  text_pause,btn_=false,127
  if death then credits() end
end

--text coroutine
function action_text(text,x,y,col,delay)
  if not high_load then
    add(actions,cocreate(function() action_text_co(text,x,y,col,delay) end))
  end
end

function action_text_co(text,x,y,col,delay)
  col,delay=col or 6,delay or 0
  if delay>0 then for i=1,delay do yield() end end
  for i=1,50 do
    local offset=i/4
    shorttext(text,x-1,y-offset,0)
    shorttext(text,x+1,y-offset,0)
    shorttext(text,x,y-1-offset,0)
    shorttext(text,x,y+1-offset,0)
    shorttext(text,x,y-offset,col)
    yield()
  end
end

function pause_game(n)
  text_pause=true
  for i=1,n do yield() end
  text_pause=false
end

--button press without auto-repeat
function btnp_(i)
  return (btn(i) and 2^i!=band(2^i,btn_))
end

-- short text by lrp
alphabet="abcdefghijklmnopqrstuvwxyz"
text_bytes={
 0x001c.1408, 0x0008.141f, 0x0014.1408, 0x001f.1408, 0x0014.140c, 0x0005.1e04,
 0x003c.5458, 0x0018.041f, 0x0000.1d00, 0x0000.1d20, 0x0014.081f, 0x0000.100f,
 0x001c.0c1c, 0x0018.041c, 0x0008.1408, 0x0018.147c, 0x007c.140c, 0x0004.0418,
 0x0004.1c10, 0x0014.0e04, 0x001c.100c, 0x000c.180c, 0x001c.181c, 0x0014.0814,
 0x003c.505c, 0x0010.1c04
}
alphadict={}
for i=1,#text_bytes do
  alphadict[sub(alphabet,i,i)]=text_bytes[i]
end
function shorttext(text,x,y,col)
  text,x,y,col,captial=text or "",x or 0,y or 0,col or 6,false
  for l=1,#text do
    letter=sub(text,l,l)
    if letter=="\\" and not capital then
      capital=true
    elseif capital or alphadict[letter]==nil then
      print(letter,x,y,col)
      x+=4
      capital=false
    else
      image=alphadict[letter]
      for i=0,31 do
        if band(image,shl(0x0000.0001,i))!=0 then
          pset(x,y+i%8,col)
        end
        if i%8==7 then x+=1 end
      end
    end
  end
end

__gfx__
50500000ccacccccccacccccccacccccccaccccccccccccccccccccccccccccccccccccc1d65544c1d65544c1285544c1285544ccccccccccccccccc00000000
05000000cc9aaacccc9aaacccc9aaacccc9aaaccc182cc12c182c12cc1a9cc19c1a9c19c1c65ddd41c65ddd41c8522241c852224cccccccccccccccc00000000
50500000ccc90accccc90accccc90accccc90accc1c88882c1c8888cc1caaaa9c1caaaacc55d2662c55d2662c5529889c5529889cccccccccccccccc000b0b00
00000000ccc9aa4cccc9aa4cccc8ea4cccc8ea4cccc80880ccc88088ccca0aa0cccaa0aac55d6666c55d6666c5528888c5528888cccccccccccccccc0000300b
00000000acc499ccacc499ccacc499ccacc499ccccce8882ccce8888ccc8aaa9ccc8aaaacc5554cccc5554cccc5554cccc5554cccccccccccccccccc00043443
0000000099499acc99499acc99499acc99499acc8c8222cccc8222ccaca999cccca999cccc65546ccc56546ccc85548ccc58548ccccccccccccccccc454400b0
00000000c999acccc999a4ccc999acccc999a4ccc22228cc8222224cc9944acca999994ccc2222cccc22221ccc1111cccc11114ccccccccccc3b3ccc00004000
00000000cc4c4cccccc4cccccc4c4cccccc4cccccc4cc4ccccc4cccccc4cc4ccccc4cccccc1cc1ccccc1cccccc4cc4ccccc4ccccc33b3bbcc33cbbcc00000000
03554400035544000355440003444430034444300035544003555400043444000355440000355440000000000000000000a000000000aa000000000000000000
0b5333400b5333400b5333400b5444b00b5444b000b533340b53334005b533300b53334000b533340000000000000000009aa000000009a00000000000000000
5531bb105531bb105531bb10055544400555444005531bb155531b0054431bb05531bb1005531bb1000000000000000000099a00000009a000000000000a0000
553bbbb0553bbbb0553bbbb005555440055554400553bbbb5553bb005543bbb0553bbbb00222bbbb00000000000000000009aa000000a990000000004a9a0000
05554000055540000555400000555430035554000055540005554000055440000355430029d92500000000000000a00000049900000499a000049494a9a90000
0355430005354300035543000354450000544530035554300535400005534000055540002d9d25304544446045555a9049499a0049499a004949a9999a900000
0111100001111000011114000011110000111100011110000111100001111400011110000292110000000000000090000999a0000999a000009999aa99000000
04004000040040000040000000400400004004000400400000440000004000000400400000240400000000000000000000000000000000000000000000000000
0000000000000000000000000000000000000000000000000000000000000000000055440000000000000000000000000000000000000000cccccccccccccccc
0000000000000000000000000000000000000000000000000000077600000000003554444000a0a0000000000000000000000000000000007cccc7cccccccccc
000000004a0000004a0000000000000000000000000505000000f7770000000000b5313310090a00000000000000000000000000000000007c5cc7cccccccccc
0808000005dd00000d88000000a9a0000000000050044440000f7f6000a9a00000553bbbb000a9a002200000000000000000440000000000775c77ccccccaccc
288f80001dd6d000288e8000099a9a00009a0000400494900007f600099a9a0005553bbdb00a9a0002ff000000000000000ff40000000000677c767cccc9accc
2288800011ddd000228880000999a90009a9a0000454444000760000092929000505553540094900000f000000e00000000f000000800000d6756dd644949acc
02220000111dd000222880000a999a00099a90000545d500010000000a99aa000055545440044000000bb0bb0eae0000000bb0bb08980000dd676d6dcccccccc
002000000111000002220000009a900000990000050505001000000000909000005555445404400000b33b3bbbe0000000b33b3bbb8000005d675665cccccccc
03bb00000000bb3000000000000000000000000000000000000000000000000005055445454b00000031333313b000000034333343b000000111000000000000
b30000000000003b00000800000000000000000000000b000000000000000000030445444004000003356311613b00000339f344f43b000011c1100000000000
b4aaaa0000aaaa4b00800000000000000000000000b0000000000000000000000b0111d110000000015555111555000004999944499900001ccc100000000000
0a07a070070a70a00020000000000820000000000030000000000b30000000000002244440000000355151155115300039949449944930000111000000000000
aa00a00aa00a00aa0000880000000000000000000000bb0000000000000000000002400240000000151011555011500049404499904490000000000000000000
aeeaaaeeeeaaaeea000002000800000000000000000003000b000000000000000002400240000000150511551105500049094499440990000000000000000070
09aa0a9009a0aa90000000000200800000000000000000000300b000000000000003000030000000110111015101500044044404940490000000000000000000
0099990000999900000000000000820082080820000000000000b300b30b0b300001000010000000000110005100000000044000940000000000000000000000
01000002200210010100000045444544033bb3b3baaabababaaabbba000008889220000000f0005533bbb3332122111200000000000000000000000000000000
021022122122212222122210225452553033303bbbbbbbbbbbbbb00b0098888882222000000000ff11b3b3112112120106ff6660000000000000000000000000
2102122110221021202222202225222201030033bbbb33b33b3301200888898882d2222000000f2f1bb131111120110260000665000000000000000000000000
11022121012121101121212122222221121010303b3333333333011088888888222222220000f0ff101111101212200100000405000000000000000000000b00
002212101011221001222221122121222211200133b300003003330388898888222222d20000f0f40102101010001011000004050000000000000b000bb0bb00
211221000222102000121211212121122212211233302222011033329888898222d22222000000f4211121021110110100000405000440000000b0000030b000
220110221222122110011000211221122211221133022121122103210888888222222220000000ff1101111111010010000004000004ff000000300b0033b0bb
01101021012112210002121022122211222122210021121122102211005001111111001000000f1101101101010110110000040000000f000bb030b0003333b0
000bb3bb33bbb3bb33b3bb0000054000bb000000bb00b05553503033bbb111bb33bbb333bbb11bbb1ddd111d1111111100000000000000000000000000000000
0b333b3bb3333bbb3bb3bbb000044000b33300000000004455500000133bbb311113bb11333bb331111d10111011000100000000000000000000000000000000
b3b323b023b22232333b33b000045000339333000000049449550000113333111111b11113333311111110111011110100000000000000000000000000000000
b03200300bb111300002223b000440003333333000004044445050001110031110113110133311111111001100011111000aaa00000088800222000000ccc000
10220302023303202201113b000540003333933300004045445050000111011100333000113310112210011111110110000aea0000008a8002a2000000cac000
222000222202201221200bb0000440003933333300000045445000001010011022122202113111102210112200011011000aaa00000088800222000000ccc000
1212222221022021011212300004500033333393000004444445000010111011110111110111101121001221100100110b00b000000b0b0000b0b0000b0b0000
10011220221111002001120000054000533533330000445554545000100111010110110111101101101101221111111000b330000000b300003b000000b30000
00000bbb22202022bb3bb33b00000000040000400b3b3bb0000000001499954410301031424424241011101001101011140414420555500000000400555504d4
0000bbba0112102233b33bbb0008200004444440b3333b330000000014449949010b310353334545181208001011110104104014555500000000040555550999
00bbbabb022110212b322b230000808004555540b221bb1300000000454999943b310b0102b44b302081121111111111142044425555000000000f50555507a7
0bbbbbbb002220101111212b0802000804000040040003400000000044949495300310b026243b501800080210101010044400440f00d0000000040055550aaa
bbabbbab0211002122022102000008000400004004000040000000701549499901b103030233b5208218888810001001141024000f00d0000000040055550999
bbbbbbbb2200022120121021020020200444444004444440000007774454449913003110053b42a2822282280101011042044124f0ffd0000000040055550000
babbabb322121011121222100080080004555540045555400000007045145441b01b0b130bb33b2b8822822810000010444010405ff500000000040055555000
bb3bb5b511102220012101220000000004000040040000400000000051454444303010b033b3b3bb288822820000000010120441555005f00000040555555000
1310333b10b03013b01330300028800000555500000000000003bbb0111000500b3bb3bbbb3bb3bbbb3bb3b02014244100000000000000100000000000000000
03a9a30131b3b103b3a9bb3100820280055005500000000000aa003b5510000033b3b33b33b3b33b33b3b33b0120114200001000000000000000010000000000
bb9a93b0333a9b13339b3ab328002000550002550444444009aa700bf25100003333330330333303303333bb4214214101000000000000000000000000000000
b92929b10b3929b1092b330b802808285000111504955940099aa0b3f55010003330000302000003020103332411422400000000000000000000000000110000
1a99abb33b99aabb1bb93b1302082022500222250459a54000990330f51010000300221222122210201020304200220000000000000000000100000001001000
01a1910301a19131bbab9333800020805011111504599540bb000b00f55000001312212221102102011221221214101200000010000000000000000001001000
1033101b1310b330b0b310b002082000522222250495594000303300551000001021112102111121221112014112412000000000010000000000010000110000
0103bb030331b1bb010301bb00080800555555550444444000333bb0515100000212021210102012101220122004214101000000000000000000000000000000
2714b41414b6000000000000460000000000000000000000000000000000000000006700000000b20000f40000000414a700000000a475d40000000087270000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
27b414b6b60000000000000056000000000067b2d400000000000000000000000000a4b40000a5a4b49585b40000140414a70000e4b7b7950000008714170000
00000000000000000000000000000006450000000000000000000000000000000000000000000095b4a400000000000000000000000005a5b400000000000000
27141400a2b2000067d200004600000000008797a70000000000000000000000000016000000000000000000000000000000000085c6c6b70000051416270000
0000000000000000000000000000675565e5c50000000000000000000000000000e4d0000000000000b6b4000000000000000000051616000000000000000000
27b41400a3b3d405152500004600000000000000000000000000000000000000000014000000e4d400004600000000e50000000000b776c60000000000270000
000000000000000000000000000087445464a70000000000000000000000f4d48797a70000b400000000b50000000005a4a5b475000000000000000000000000
2714b4b4a4958514b414a70046000000000000000000000000000000000000000000160000b4a495b4145614000000a4b416e200000076760095b4b400070000
00000000000000000000000000001414b414140010000000000000000000879716000000001400000000140000000516000000b44242500000f4000000000000
27b7b7b714b4141414b414f44600000000000000000000000000000000000000000014007484b5b600004600000000001416040000427676000000b500270000
000000000000000006450000000000b61424b48595b4a400000000000000000000000000001457100067140000051600e40000b495b4b4b475a7000000000000
27c6b716b4b4b4b4b4b4b4a74600000000000000000000000000000000000000000000d09477b500000046000000000000460000b7c67676000000b600170000
000000000000e4425565f40000000000b6b41604b42416b400000000000000000000000000b6b4a585a4b60000000000854267b4000000000000000000000000
27c6b7b6350000003500003525000000000000000000000000000000000000000000a4b4b4b4b600000046000000000000460000c6767676e200000000270000
00000000000087979797a700000000000014b4141414b4140000000000000000000000000000000000000075e4100000b4b4a4b4000000000000000000000000
27b7000035000000350000350046000000000000000000e467f40000000000000000000000000000a2d246001414000000567676767676767676760000270000
000000460000000000000000000000000016142400000014b4000000000000000000000000000000000000b6a4b4a50000000000000000000000000000000000
27c60000350000003500003500560000000000000000879797a70000000000000000000000000000a3b34614b414000000467600c67676c6c676000000070000
00b495560000000000000000000000000000000000000000000000000000000000000000d4e40000000000000000000000000000000000000000000000000000
27c600000000000000000000004600000000000000000000000000000000000000000000b200b4857595a5141414000000460000000000000076760000270000
d4b50046000000000000000000000000000000000000b4b4b4b70000000000000000001695a40000000000000000000000000000000000000000000000000000
27b70000e2e2e2e5e2e2e200004600000000000000000000000000000000000000000085b4b414a5b6b6000014b40000004600000000000000000000002700b4
a4b60046000000000000000000000000000000000000007484b700000000000000d4000000000000000000000000000087000000a70000000000000000000000
27c66700b7b7b7b7b7b7b7b400460000f4e4c5b200e400e4f40000c5d5e4000000c2b41414b6b60000000000141434b4343434b434b4b4950000d40087277500
00000046000000000000000000000000e55700d4e410429477b7b40000000016b485000000000000000000f510008797b4000000b497a70000f5000000000000
27b72600b7b7b7b7b7b7b7b5b4b43434449744974497449797444444546414b4a485b6b6b6000000000000000014b7b4b4b7b7b7b4b7b4b7b4b4a4b4b727b5b2
425042460000000000000000d2e2e28744974497879744a7974444a700000000000000d40000000000d487979797b400004267000000b49797a7000000000000
2700004600000000000000000000000000004600000000000000000000000000000000000000000000000000000000000000000000000000000000000017b595
a4b4b485b4b4a4a5b4a5b4b4a49797b4b4b4b4b4b4b4b4b4b4b4b4b4b4000000000000a4b4b4160000b4b4b4b400000000b4b40000000000b4b4000000000000
07000046000000000000000000000000000046000000000000000000000000000000000000000000000000000000000000000000000000000000000000270000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
27000046000000c5e400504200000000e4e54610e400000000000000000000000000000000000000000000000000000000000000000000000000000000071727
17272707270727171727271707072707270727072707170707170717271707172707271707071707272717070000000000000000000000000000000000000000
17000046000000051515152500000000051526262500000000c5f4b200000000000000000000000000000000000000000000c2b2000000000000000000272786
86868686866700000000008686178686868686860086864242424242000000008686868600000000868686070000000000000000000000000000000000000000
2700004600000000000000000000000000000000000000000005152500000000000000000000000000000000000000f542d5c3d367f510e50000000000271700
00000000868686860000000000078686868686868686868686868686000000000086868647000000000086070000000000000000000000000000000000000000
2700e446b20000000000000000000000000000000000000000000000000000000000000000000000000000000000008744544464546454a70000000000271786
00000067868686868686000000178686008686868686868686868686860000000000868686860000000000070000000000000000000000000000000000000000
0700051526250000000000000000000000000000000000000000000000000000000000000000c500e500d200000000b6b5b5b5b5b5b6b6b60000000000272786
42000086868686868686000086170086000000868600008686860000868600000000000000000000868686070000000000000000000000000000000000000000
270000000000000000000000000000000000000000000000000000000000000000000000879744546497a70000000000b6b6b6b6b60000000000b4b400271786
86000086000000860000000086278686860000000000000000000000868600000000000000000086868686070000000000000000000000000000000000000000
1700000000000000000000000000000000000000000000000000c500000000000000b20000005700000014250000000000000000000000000000d4b600172700
00000000000000000000868686178686000000000000000000000086860000000000000086868686868686170000000000000000000000000000000000000000
270000000000d200f400000000000000000000000000f5008797a7e5000000008797a70000b7140000000014f5000000000000004200000000b4a4a500072700
00000000000000000000000086170000000000000000000000000000000000000000000000000086868686070000000000000000000000000000000000000000
2700000000051515152500000000000000000000d500879714161697a700000000000000001400000000001425000000000000a5a40000000000000000270700
00000000000000000000000000170000000000000000000000000000000000000000000000000000868686070000000000000000000000000000000000000000
0700000005161404241425b20000000000000000879714b414000014b4a7e5000000000014b7000000000000b700000000000000000000000000000000271700
00000000000000900000000000270000000000000000000000000000000000008686000000000000008600170000000000000000000000000000000000000000
2700000514424242424214250000000000c5008714b41400000000000014a70000000000b700000000000000c6a7000010000000000000000010005700271700
00000000f5e4e596d5f4000000070000000000000000000000000000000000000086000000000000000000070000000000000000000000000000000000000000
2700e42414b4a5a514a514160000000000879714160000000000006700b4140057000000c6004210d46767c2c6b7b4b4b4a4b495b485a4b4a4b4b434341727d0
010000008744549764a700470027000000000000000000000000000000000000000000000000000000000017000000000000000000c4d6000000064500000000
272626a5a514a504b4a5a51426262626261414b4000000000000a5857514b4141414b71414b4b4b495a485a4b714a514a5a5a514a5a514a51414a5a5a5071776
76767676c6b7c6c6c6c6767676070000000100000000000000b00000000000000000000000000000d202c22700e500000001000000e6f6520013556500000000
27a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6271776
7676c6c6b7b7c676767676767607b4a5b4b4b4a5b4a5b4b4a5b4a5b4b4a5a5b4b4a5b4b4a5b4b4b47585a40797979787979797a7b4b4a5b487a7979787000000
17271717270717272727271727171707172727272717171727271727272717072717271727270717272727171727270727272717272717270727172727271776
76c6c6b7b7b7b7c6767676767627a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a6a607b4b4b4b4b4b4b4b4b4b4b4b4b4b4b4b4b4000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001727
1727270727072717172727170707270727072707270717070717071727170717270727170707170727271707b6b6b6b6b6b6b6b6b6b6b6b6b6b6b6b6b6000000
__label__
00000000001030103110301031000000000000000000000000212211122220202233bbb333000000000000000000000000000000000000000000f00055111000
0000000000010b3103010b3103000000000000000000000000211212010112102211b3b3110000000000021028100000000000000000000000000000ff551000
00000000003b310b013b310b0100000000000000000000000011201102022110211bb13111000000000008888010000000000000000000000000000f2ff25100
0000000000300310b0300310b000000000000000000220000012122001002220101011111000000000008808800000000000000000000000000000f0fff55010
000000000001b1030301b10303000000000000000002ff000010001011021100210102101000000000008888e000000000009a0000009a00000000f0f4f51010
000000000013003110130031100000000000000000000f000011101101220002212111210200000000000022280000000009a9a00009a9a000000000f4f55000
0000000000b01b0b13b01b0b130000000000000000000bb0bb110100102212101111011111000000000004222228000000099a9000099a9000000000ff551000
0000000000303010b0303010b0000000000000000000b33b3b010110111110222001101101000000000000004000000000009900000099000000000f11515100
00000000000000000010301031000000000000000033bbb333222020222122111221221112212211122220202221221112222020221ddd111d33bbb333bbb111
000000000000000000010b310300000000000000001113bb1101121022211212012112120121121201011210222112120101121022111d101111b3b311133bbb
0000000000000000003b310b0100000000000000001111b11102211021112011021120110211201102022110211120110202211021111110111bb13111113333
000000000000000000300310b0000000000000000010113110002220101212200112122001121220010022201012122001002220101111001110111110111003
00000000000000000001b10303000000000000000000333000021100211000101110001011100010110211002110001011021100212210011101021010011101
00000000000000000013003110000000000000000022122202220002211110110111101101111011012200022111101101220002212210112221112102101001
000000000000000000b01b0b13000000000000000011011111221210111101001011010010110100102212101111010010221210112100122111011111101110
000000000000000000303010b0000000000000000001101101111022200101101101011011010110111110222001011011111022201011012201101101100111
00000000000000000010301031000000100000000000000000000000000400004020142441222020221030103122202022222020221030103122202022222020
000000000000000000010b310300000000000000000000000000000000044444400120114201121022010b31030112102201121022010b310301121022011210
0000000000000000003b310b01000000000000000000000000000000000455554042142141022110213b310b0102211021022110213b310b0102211021022110
000000000000000000300310b000000000000000000000000000000000040000402411422400222010300310b00022201000222010300310b000222010002220
000000a9000000000001b103030000000000000000000000000000000004000040420022000211002101b10303021100210211002101b1030302110021021100
00000a9a900000000013003110000000000000000000000000000440000444444012141012220002211300311022000221220002211300311022000221220002
000009a99000000000b01b0b130100000000000000000000000004ff00045555404112412022121011b01b0b132212101122121011b01b0b1322121011221210
000000990000000000303010b000000000000000000000000000000f00040000402004214111102220303010b01110222011102220303010b011102220111022
31103010311030103110301031000000002220202220021001222020220b3b3bb022202022201424412122111210301031103010310100000220021001000000
03010b3103010b3103010b310300000000011210222122212201121022b3333b33011210220120114221121201010b3103010b31030210221221222122000000
013b310b013b310b013b310b0100000000022110211022102102211021b221bb130221102142142141112011023b310b013b310b012102122110221021000000
b0300310b0300310b0300310b00000000000222010012121100022201004000340002220102411422412122001300310b0300310b01102212101212110000000
0301b1030301b1030301b10303000000000211002110112210021100210400004002110021420022001000101101b1030301b103030022121010112210000000
10130031101300311013003110000000002200022102221020220002210444444022000221121410121110110113003110130031102112210002221020000000
13b01b0b13b01b0b13b01b0b130000000022121011122212212212101104555540221210114112412011010010b01b0b13b01b0b132201102212221221000000
b0303010b0303010b0303010b00000000011102220012112211110222004000040111022202004214101011011303010b0303010b00110102101211221000000
3110301031103010310000000000000003bb02100122202022222020220400004022202022201424412220202222202022103010312014244121221112000000
03010b3103010b310300000000000000b30000000000000000000000000000000000000000000000000000102201121022010b31030120114221121201000000
013b310b013b310b0100000000000000b4aaaa0000a000aaa0aaa00aa0aa00a0a0a0a0aa00aaa0aaa0aaa01021022110213b310b014214214111201102000000
b0300310b0300310b0000000000000000a07a07000a000a000aaa0a0a0a0a0a0a0a0a0a0a00a00a000a0a0201000222010300310b02411422412122001000000
0301b1030301b1030300000000000000aa00a00a00a000aa00a0a0a0a0a0a0aaa0a0a0a0a00a00aa00aa0000210211002101b103034200220010001011000000
10130031101300311000000000000000aeeaaaee00a000a000a0a0a0a0a0a0a0a0a0a0a0a00a00a000a0a0022122000221130031101214101211101101000000
13b01b0b13b01b0b130000000000000009aa0a9000aaa0aaa0a0a0aa00a0a0a0a00aa0a0a00a00aaa0a0a0101122121011b01b0b134112412011010010000000
b0303010b0303010b000000000000000009999000000000000000000000000000000000000000000000000222011102220303010b02004214101011011000000
31103010311030103100000000000000002122111200000000000000000400004000000000222020220000000022202022201424412220202200000000000000
03010b3103010b310300000000000000002112120100000000000000000444444000000000011210220000000001121022012011420112102200000000000000
013b310b013b310b0100000000000000001120110200000000000000000455554000000000022110210000000002211021421421410221102100000000000000
b0300310b0300310b000000000000000001212200100000000000000000400004000000000002220100000000000222010241142240022201002220000000000
0301b1030301b1030300000000000000001000101100000000000000000400004000000000021100210000000002110021420022000211002102a20000000000
10130031101300311000000000000000001110110100000000000000000444444000000000220002210004400022000221121410122200022102220000000000
13b01b0b13b01b0b1300000000000000001101001000000000000000000455554000000000221210110004ff0022121011411241202212101100b0b000000000
b0303010b0303010b0000000000000000001011011000000000000000004000040000000001110222000000f00111022202004214111102220003b0000000000
31103010310000000000000000000000001ddd111d00000000000000000400004000000000000000002220202222202022010000022220202222202022000000
03010b3103000000000000000000000000111d101100000000000000000444444000000000000000000112102201121022021022120112102201121022000000
013b310b010000000000000000000000001111101100000000000000000455554000000000000000000221102102211021210212210221102102211021000000
b0300310b00000000000000000000000001111001102200000000000000400004000000000000000000022201000222010110221210022201000222010000000
0301b103030000000000000000000000002210011102ff0000000000000400004000000000000000000211002102110021002212100211002102110021000000
101300311000000000000000000000000022101122000f0000008000000444444000000000000000002200022122000221211221002200022122000221000000
13b01b0b1300000000000000000000000021001221000bb0bb089800000455554000000000000000002212101122121011220110222212101122121011000000
b0303010b00000000000000000000000001011012200b33b3bbb8000000400004000000000000000001110222011102220011010211110222011102220000000
31000000000000000000000010000000001ddd111d0031333313b00000000000000003bbb0000000000000000000000000000000000000000000000000000000
0300000100000000000000000000000000111d101103356311613b00000000000000aa003b000000000000000000000000000000000000000000000000000000
01000000000000000000000000000000001111101101555511155500000000000009aa700b000000000000000000000000000000000000000000000000000000
b00000000000000000000000000000000011110011355151155115300000000000099aa0b3000000000000000000000000000000000000000000000000000000
03010000000000000000000000000000002210011115101155501150000000000000990330000000000000000000000000000000000000000000000000000000
100000000000000000000000000000000022101122150511551105500000000000bb000b00000000000000000000000000000000000000000000000000000000
13000001000000000001000000000000002100122111011101510150000000000000303300000000000000000000000000000000000000000000000000000000
b0000000000000000000000000000000001011012200011000510000000000000000333bb0000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000110101133bbb333bbb111bb2122111233bbb333000000000000000000000000000000000000000000000000000000
0000000000000000000000000000000000101111011113bb11133bbb312112120111b3b311000000000000000000000000000000000000000000000100000000
0000000000000000000000000000000000111111111111b11111333311112011021bb13111000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000001010101010113110111003111212200110111110000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000001000100100333000011101111000101101021010000000000000000000000000000000000000000001000000000000
00000000000000000000000000000000000101011022122202101001101110110121112102000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000001000001011011111101110111101001011011111000000000000000000000000000000000000000000000100000000
00000000000000000000000000000000000000000001101101100111010101101101101101000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00022200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0002a2000000000b0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00022200000000b00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0000b0b0000000300b00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00003b00000bb030b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
bb0b3bb3bbbb3bb33bbb3bb33b000000000000000000a00000000000000000000000000000000000000000000000000000000000000000000000000000000000
3b33b3b33b33b33bbb33b33bbb0000000000000000009aaa00000000000000000000000000000000000000000000000000000000000000000000000000000000
03333333032b322b232b322b23000000000000000000090a00000000000000000000000000000000000000000000000000000000000000000000000000044444
03333000031111212b1111212b00000000000000000009aa40000000000000000000000000000000000000000000000000000000000000000000000000049559
100300221222022102220221020000000000000000a0049900000000000000000000000b00000000000000000000000000000000000000000000000000045a95
02131221222012102120121021000000000000000099499a0000000000000000000000b000000000000000000000000000000000000000000000000000045995
2110211121121222101212221000000000000000000999a40000000000000000000000300b000000000000000000000000000000000000000000000000049559
1202120212012101220121012200000000000000000004000000000000000000000bb030b0000000000000000000000000000000000000000000000000044444
12212211122014244120142441212211122122111233bbb3332122111233bbb33333bbb333212211122122111221221112bbb111bbbbb11bbb21221112212211
01211212010120114201201142211212012112120111b3b311211212011113bb1111b3b311211212012112120121121201133bbb31333bb33121121201211212
0211201102421421414214214111201102112011021bb13111112011021111b1111bb13111112011021120110211201102113333111333331111201102112011
01121220012411422424114224121220011212200110111110121220011011311010111110121220011212200112122001111003111333111112122001121220
11100010114200220042002200100010111000101101021010100010110033300001021010100010111000101110001011011101111133101110001011100010
01111011011214101212141012111011011110110121112102111011012212220221112102111011011110110111101101101001101131111011101101111011
10110100104112412041124120110100101101001011011111110100101101111111011111110100101101001011010010101110110111101111010010110100
11010110112004214120042141010110110101101101101101010110110110110101101101010110110101101101011011100111011110110101011011010110
22200210012220202222202022000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
22212221220112102201121022000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000
21102210210221102102211021000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
10012121100022201000222010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
21101122100211002102110021000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000
21022210202200022122000221000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
11122212212212101122121011000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000
20012112211110222011102220000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
222122111222202022000000000000000000000000000000000000000000000000000000000000000000000000000000100000000000000000bbb11bbb212211
222112120101121022000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000333bb331211212
21112011020221102100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000013333311112011
10121220010022201000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000013331111121220
21100010110211002100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011331011100010
21111055442200022100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011311110111011
11113554444212a0a100000000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000001111011110100
2001b5313311192a2000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011101101010110
0020553bbbb220a9a200000000000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000
1025553bbdb11a9a2200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
20152555354229492100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
21015554544024401000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00005555445404400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
0005055445454b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00030445444004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000b0111d11000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000224444000000000000000000000000000000055505550550055500000000000005000500005000000000000000000000000000000000000000000000000
00000240024000000000000000000000000000000000505050050000500000000000005000500000000000000000000000000000000000000000000000000000
00000240024000000000000000000000000000000055505050050000500000555000005500500005005500550055000000000000000000000000000000000000
00000300003000000000000000000000000000000050005050050000500000000000005050500005000500050005000000000000000000000000000000000000
00000100001000000000000000000000000000000055505550555000500000000000005500050005000550055005500000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

__gff__
000000000000000000000000000000000000000000000000000000000000000000000000000000000000c0c0c0c000000000000000000000000080808080000084848484848484c0c080848400c0c0c084848408c080808484848484c0c0c0c0c084840008180084848404848400000004040400400400808484848400000000
0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__map__
70000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000716168680068684b0000000000006861000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
7241614b4140414b414061416140616141404161616161686842000000404061416141614161416161426141616100000000000000000000000000000072616800000000530000005f000000410000002e2e2e2e2e00000076686800000000000000000000000000000000000000000000000000000000000000000000000000
7241000000000000000068616861686141616168686168680000240064000000002d0064004e00006141614b6100000000000000004e000000000000007161000000642b53000078517a00006100000040404040400000686868680000000000004d000000000047480000000000000000000000000000000000000000000000
726100000000000000000068686140414b61006800000000000050626500005062626265625200004d4b4241610000000000004e2a502e2e2e524d4f007061000000655153000061680000004100000053535353530000006868000000002a4b614a000500242449772400000000000000000000000000000000000000000000
7161000000507a000000000000004161416100005d4e5c00000000006400006100000064006100505141614b6100000000505151516141614b4b4b51517261005c006461530000610000000042000000005300005300000000000000000058614b4b4b614b615a4a574b0000000000000000004e4f4e054f0000000000000000
7161105d0d4e614f756400000024614b610000005045465200000000640000414f764e00016164000000006100000000004e00000000005e00686868417261515200646100104d410001244e612d7400000000000000004d050064000000004d647b6168616168616100000000000000000000787979447a0000000000000000
7041515151514b515265000000504b614b000000616168000000002d64004d615151515151616500000000000000000050515152000000510000000061716142000064614b594a4b006151446157584b0000000000004b4a4b5865000061416165617b4b6868404100000000000000000000006b6b6b6b6b0000000000000000
726b6b6b41426b00006400000000614061000000614b000000005057585759610000614161416400000000000000000068686861014f24414f4d057641700000000064000000000000000000006b6b000000000000006b6b5b6b64000041616164617b6161687b4b000000000000000000000000000000000000000000000000
7200000005000000006400000000000061520000616100100000615b5b5a5a414d24004d000d64004a4b00000000000000000061416141616141614161710000000064000000000000000000000000640001004d000000006b006400004b00006400614d617b615e0000000000004e240d000000000000000000000000000000
71000000505100000064000000000000000000004b615957585a616b6b6b6b614a416158416164005b414b2c0000000000000000006800006868680000710000000d645c014e000064004f5f4e0000654b4b4b4a0000000000006400005a2a2d6400006161406161000000000050516252000000000000000000000000000000
7200002b01005f00006400000000000000004e0000000000000000000000000000006140426164005a6161584a4b0000000000000000000000000000007200505151515151515151657979797a0000646b5b5b6b004e000005006400005a3a3b0076000000000000000000000000000000000000000000000000000000000000
7100505151515200006400000000000000505164004e00754e4d004f01004e0000000061406164616168680000000000004e5c4e005e4f00000000000071504b68006868680024686400686800000064006b6b0000584b4a4b595700006b58574b4a000000000000000000000000000000000000000000000000000000000000
7200000000000000006400000000002b5c416165506262626262626262627a5e00000061610064000000000000000000506262626262625200004e000072005e00000000680068006400750000000064000000000000000000000000000000000000000000000000000000000000000000000d4e4f5c4f4e5f4f000000000000
71000000000000000064000000005079506161640000000000004d004f00005800004e012c4e0000000100004e2d410000000000000000615f00505151725a68006868000d000000646868685a4f5e640d4e5d2b00005f4f2d4e002a00000000004e000001000000750000004f4e005062626262626262626262000000000000
7052004f0001004e00640d005e50616140614d4e010d4f00004e5062524d245b4b6279794462794462447944447961414e0051012d764e616262614161716b575a594a68686868686868684a6b6262626262626262626262626262624b4b4a4b584a4b4b4b57594b4b574b626262624b6b000000000000000000000000000000
724062794462796279624479446161616161626262626262626261616162626b6b6b6b6b61614141616141616161416162626162626262614161614161710000000000000000000000000d4e00000000000068000000006800000000000000000000000000000000000000000000000000000000000000000000000000000000
71000000000000000000000000000000415a415a5b414b414b415b4b41414100000000000000000000000000000000000000000000000000000000000070000000000000004b4b5a4b59584a2400680000686868682468680068680000000000000000000000000000000000000000000000002c2d0000000000000000000000
720000000000000000000000000000005b412a2b0000415b41414141405b41000024050000000000000000000000000000000000000000000000000000724d0000000000000000530053004b58002468006868686868686868686868000000000000000000000000000000000000000000000d3c3d764e5f0000000000000000
70000068686800000000000000000000415a3a3b004e00002d00645b414141004b414b4b4b0000006400244d0000000000000000000000000000000000714a2a4e05004e4d0000530053007b6b0068686868680000000000000068680000000000000000004b4b4b000000000000000000007879454679520000000000000000
71000068680000000000000000006800415b594a4b584b4b4a416541414b41006b6b000000000000654b4b4b0000000000000000000000000000000000714b4b4b5a4b4b5a4b00000053006c0000686800680000000000000000006800000000000000000053005a4b0000000000000000000000005300000000000000000000
72002468682b760000000000006868004b41000000000000004164415b4141000000000000000000640000000000000000000000000000000000000000726b5b00000000000000000000007b0068682b6868680000000000762405680000000000000024005300005a2400000000000000000000005300000000000000000000
71006868686868686800000068686868415b7600000000007640644b41414000000000000000000064000000000000000000000000000000000000000071005a00000000000000002e2e2e6c000068686868680000006868686868680000000000000059005300007b7b4b000000000000000000005300000000000000000000
710068686868686800000000006800684b415924762476245a4164414b41410000000000000000006400000000000000000064004d4e0500742400000072005b010000750000005a7b7b7b7b006868686868000000000d68686868000000000000014d410053000000007b000000000000000000005300000000000000000000
7200680068680000000000006868000041414b5a585a57594b41644140414100000000000000000064000064000000000000655051515151515200000070006b5b5a5b5a4b0000000000000000006868680000000068686868686800000000004b57587b0053000000057b0000000000010000002b5300000000000000000000
70006800000000000000000000000000414b5b6b6b6b6b6b6b41644141404100004d414b4b004b4b4b4b416500000000000064616142614061610000007100000000000000000000000000000000006868686800000068686868000000000059404100000053007b7b7b7b0000004b574b4a4b4b585300000000000000000000
720000000000000000686800000000004041410000000000000064004241410000414b0000000000000041640000000000006442614061615a420000007000000000000000787a00000000000000006868682d00000000686800000000004d414b000000005300006b6b000000005a5b425a5b6b005300000000000000000000
72000000000000000068000000000000000000000d4d4f004e5e64000000000000414000052b0005642a4164000000000000646b6b6b6b6b6b6b00000070000000000078794b6b7a000000000000000000686800000000000000000000004a4b0000000000530000000000004d4b5b5a415b6b00005300000000000000000000
710000000000000000000000670000000000002a7879797979797a0000000000414041614b594b41654a41640000000000006400000000000000000000710000004d784b6b6b007b004d004d00005f00000000004e014f5c005e4e004b574b0000000000000000000000004b4a5a415b5a5a0000000000004e00000000000000
71000000000000000000000067670000414043434b41614b4142414343414141415a427674004d0064414b4b4100000000006400000000000000000000714b4b4b4b4b6b0000006b4b7b4b7b4b62522c0d5e4f00507978447978626241424e2476242b2e2e2e4b4b005c005b6b6b6b6b6b4b0000242e004d4a4b000000000000
70002a592e4a00755e0000006767004e41414b4b414b404b41414b4b4b41410047484b4a41415a414140415a4041004d005d640005004e000000054d00716b6b00000000000000000000006b6b7b7b79797979797b7b4b7b4b4b7b7b7b7b584445464a6b6b6b6b4b4b4a576b00000000006b00004b4b5758005b000000000000
724b585b5b5b4b6244505162434362514141614141424141414141414041002449775b5a405b41405b41415b414162626262626262626262626262626272000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
72414b414b410000000000000000000000000000000000000000000000000000000000000000000000000000000041520000000000760000000000000070000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__sfx__
01020000126371862719627166071d6071f607086071c6070c6071560715607136071360712607106070f6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c6070c607
00010000124550e4120d4150e41210415184521f4551f452274550840206405044020340502402034050440223405234022b4052b40223405234022b4052b40223405234022a4052a40223405234022a4052a402
0002000032337373573b3673b35731357353473b337373373032736317333173231705307053070c3070e3070430710307103070e307003070c3070c3070e3070430710307103070e307003070c3070c3070e307
002700001766327613126032a6032d6032f603306033160331603316033160330603306032f6032b603246031c0031c00328003280031c0031c00328003280031c0031c00328003280031c0031c0032800328003
000300000a75607736067260571605716007060070600706007060070600706007060070600706007060070600706007060070600706007060070600706007060070600706007060070600706007060070600706
0004000007725087250a7150b7111c70513705107050e7050d7011a7050d7050b7050970507701077050070500705007050070500705007050070500705007050070500705007050070500705007050070500705
000b00002323300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003
000300002f7453474537700267002d700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011500001c0341d04428044280221c0001d0001c0051d0051c0050000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
000c00002e12239142001070010700107001070010700107001070010700107001070010700107001070010700107001070010700107001070010700107001070010700107001070010700107001070010700107
010600000161406620000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
011400002224332253372332c223282132d2022320217202002030020300203002030020300203002030020300203002030020300203002030020300203002030020300203002030020300203002030020300203
01100000132032c2543523235222352122a2000020000200002000020000200002000020000200002000020000200002000020000200002000020000200002000020000200002000020000200002000020000200
010e00001d340223402b3522b32200300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300003000030000300
010e0000223502e3652d3062b30600306003060030600306003060030600306003060030600306003060030600306003060030600306003060030600306003060030600306003060030600306003060030600306
001000002b35424354323440030400304003040030400304003040030400304003040030400304003040030400304003040030400304003040030400304003040030400304003040030400304003040030400304
00120000181201a1301b1301714016150061402a140041402a1400414029140281302612024120101300f13000100001000010000100001000010000100001000010000100001000010000100001000010000100
00100000131401b14011140101401d1400f1400e1401d1400e1401c1400e1401a1400f140191401014012140141401b1401c14017140000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
__music__
01 00024344
02 00020144
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
00 41424344
