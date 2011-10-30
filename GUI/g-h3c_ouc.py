# g-h3c_ouc.py
#encoding:utf-8

import wx
import time
import threading
import subprocess,sys,urllib,httplib

devicename = 'eth0'
statusbar = None
tc = None

#Set the Statusbar.
class MyStatusBar(wx.StatusBar):
    def __init__(self,parent):
        wx.StatusBar.__init__(self,parent)

        self.SetFieldsCount(2)
        self.SetStatusText('Welcome to Client',0)
        self.SetStatusWidths([-5,-2])
	self.SetStatusText(time.strftime("%I:%M:%S",time.localtime(time.time())),1)
	self.timer = wx.Timer(self)
	self.Bind(wx.EVT_TIMER,self.OnTimer,self.timer)
	self.timer.Start(1000)

    def Draw(self, dc):
        t = time.localtime(time.time())
        st = time.strftime("%I:%M:%S", t)
	self.SetStatusText(st,1)
	self.SetFieldsCount(2)
	self.SetStatusWidths([-5,-2])
	

    def OnTimer(self,evt):
	dc = wx.BufferedDC(wx.ClientDC(self))
	self.Draw(dc)
   
    def OnPaint(self, evt):
        dc = wx.BufferedPaintDC(self)
        self.Draw(dc)
        
#Set the Taskbar.
class TaskBarIcon(wx.TaskBarIcon):
    ID_Bar = wx.NewId()
    def __init__(self, frame):
        wx.TaskBarIcon.__init__(self)
        self.frame = frame
        self.SetIcon(wx.Icon(name='disconnected.png', type=wx.BITMAP_TYPE_PNG), 'TaskBarIcon!')
        self.Bind(wx.EVT_TASKBAR_LEFT_DOWN, self.OnLeftDown)
	self.Bind(wx.EVT_TASKBAR_RIGHT_DOWN, self.OnRightDown)

    def OnLeftDown(self, event):
        if self.frame.Show():
		self.frame.Show(True)
	else:
		self.frame.Show(False)
        
    def OnRightDown(self, event):
	if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
	    self.popupID2 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnPopupOne, id=self.popupID1)
	    self.Bind(wx.EVT_MENU, self.OnPopupTwo, id=self.popupID2)
        menu = wx.Menu()
        menu.Append(self.popupID1, "Exit")
	menu.Append(self.popupID2, "Disconnect")
        self.PopupMenu(menu)
        menu.Destroy()

    def OnPopupOne(self, event):
	global devicename
	dlg = wx.MessageDialog(None,'Do you really wanna exit the Application?','Message Box',wx.YES_NO | wx.ICON_QUESTION)
	retCode = dlg.ShowModal()
	if retCode == wx.ID_YES:
		subprocess.Popen(['./h3c_ouc','-l',devicename],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
        	self.RemoveIcon()
		self.frame.Destroy()
	else:
		pass
	dlg.Destroy()
	
    def OnPopupTwo(self, event):
	global devicename
	global statusbar
	global tc
	p=subprocess.Popen(['./h3c_ouc','-l',devicename],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=False)
	out=p.stdout.read()
        tc.AppendText(out)
	statusbar.SetStatusText('Disconnected.')
	self.SetIcon(wx.Icon(name='disconnected.png', type=wx.BITMAP_TYPE_PNG))

username = ''
password = ''
state = False
class MyFrame(wx.Frame):
    def __init__(self,parent,id,title):
	self.cfg = wx.Config('myconfig')
	if self.cfg.Exists('username'):
		username, password,state = self.cfg.Read('username'), self.cfg.Read('password'), self.cfg.ReadBool('state')
	else:
		username = ''
		password = ''
		state = False
        wx.Frame.__init__(self,parent,id,title,size=(420,480),style=wx.DEFAULT_FRAME_STYLE)
	global statusbar
	global tc
	self.taskBarIcon = TaskBarIcon(self)
	statusbar = MyStatusBar(self)
        self.SetStatusBar(statusbar)

	menubar = wx.MenuBar() 
	file = wx.Menu()
	help = wx.Menu()
        file.Append(101, '&Disconnect', 'Disconnect the Net.')
        file.AppendSeparator()
        quit = wx.MenuItem(file, 102, '&Quit\tCtrl+Q', 'Quit the Application')
        file.AppendItem(quit)
	help.Append(103, '&About', 'Information about the Application.')

  	menubar.Append(file, '&File')
        menubar.Append(help, '&Help')

	self.SetMenuBar(menubar)
	
	hbox  = wx.BoxSizer(wx.HORIZONTAL)
        vbox1 = wx.BoxSizer(wx.VERTICAL)
        vbox2 = wx.GridSizer(4,2,1,4)
        hbox3 = wx.BoxSizer(wx.HORIZONTAL)
	vbox4 = wx.BoxSizer(wx.VERTICAL)
	panel1 = wx.Panel(self,-1)
	panel2 = wx.Panel(self,-1)
	panel3 = wx.Panel(self,-1)

	vbox1.Add(panel1, 2, wx.EXPAND | wx.ALL, 3)
        vbox1.Add(panel2, 1, wx.EXPAND | wx.ALL, 3)
	vbox1.Add(panel3, 4, wx.EXPAND | wx.ALL, 3)

        self.UserName = wx.TextCtrl(panel1,-1,str(username),style=wx.TE_PROCESS_ENTER)
        self.Password = wx.TextCtrl(panel1,-1,str(password),style=wx.TE_PASSWORD)
	self.author = ['eth0','lo']
	self.choice = wx.Choice(panel1, -1,choices=['eth0','lo'], style=wx.CB_READONLY)
	self.rp = wx.CheckBox(panel1,2,'Remember Password')
	self.rp.SetValue(state)
	tc = wx.TextCtrl(panel3, -1, style=wx.TE_MULTILINE | wx.TE_READONLY)
        vbox2.AddMany([ (wx.StaticText(panel1, -1, 'Username'),1, wx.LEFT,15),
                        (self.UserName, 2, wx.RIGHT|wx.EXPAND,15),
                        (wx.StaticText(panel1, -1, 'Password'),1, wx.LEFT,15),
                        (self.Password, 2, wx.RIGHT|wx.EXPAND,15),
			(wx.StaticText(panel1, -1, 'Devicename'),1, wx.LEFT,15),			
			(self.choice,2, wx.RIGHT|wx.EXPAND,15),
			(self.rp,1,wx.LEFT,15)])
	panel1.SetSizer(vbox2)

        hbox3.Add(wx.Button(panel2, 3, 'Connect'), 1, wx.ALL, 15)
        hbox3.Add(wx.Button(panel2, 4, 'Disconnect'), 1, wx.ALL, 15)
	panel2.SetSizer(hbox3)

	vbox4.Add(tc,1,wx.ALL | wx.EXPAND, 15)
	panel3.SetSizer(vbox4)
	
	self.Bind(wx.EVT_CLOSE, self.OnQuit)
	self.Bind(wx.EVT_CHECKBOX,self.RePassword,id=2)
	self.Bind(wx.EVT_BUTTON,self.OnConnect,id=3)
        self.Bind(wx.EVT_BUTTON,self.OnDisConnect,id=4)
	self.Bind(wx.EVT_COMBOBOX, self.OnSelect)
	self.Bind(wx.EVT_MENU, self.OnDisConnect, id=101)
	self.Bind(wx.EVT_MENU, self.OnQuit, id=102)
	self.Bind(wx.EVT_MENU, self.OnAbout, id=103)

        hbox.Add(vbox1, 1, wx.EXPAND)
        self.SetSizer(hbox)
        self.Centre()

    def OnSelect(self, event):			      
	item = event.GetSelection()

    def OnQuit(self, event):
	global devicename
	dlg = wx.MessageDialog(None,'Do you really wanna exit the Application?','Message Box',wx.YES_NO | wx.ICON_QUESTION)
	retCode = dlg.ShowModal()
	if retCode == wx.ID_YES:
		subprocess.Popen(['./h3c_ouc','-l',devicename],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
		self.taskBarIcon.Destroy()
		self.Destroy() 
	else:
		pass
	dlg.Destroy()    

    def OnAbout(self, event):
	text = 'This is a Net connection program.\nThank you for your usage.\nUpdate on http://www.ouc.edu.cn\nCopyright(c)Ocean University of China'
        dlg = wx.MessageDialog(self, text, 'About',wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

    def RePassword(self, event):
	if self.rp.GetValue():
		self.cfg.Write("username", self.UserName.GetValue())
		self.cfg.Write("password", self.Password.GetValue())
		self.cfg.WriteBool("state",True)
	else:
		self.cfg.Write("username", '')
		self.cfg.Write("password", '')
		self.cfg.WriteBool("state",False)

    def OnConnect(self, event):			#Connect the Internet.	
	global devicename	
	username = self.UserName.GetValue()
       	password = self.Password.GetValue()
	devicename = self.author[self.choice.GetSelection()]
        cli=ThreadCli(username,password,devicename,self)
	cli.setDaemon(True)
	cli.start()

    def OnDisConnect(self,event):		#Disconnect the Internet.
	global devicename
	p=subprocess.Popen(['./h3c_ouc','-l',devicename],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
	out=p.stdout.readline()
	self.Display(out)
	self.DDisplay()

    def Display(self,msg):
        tc.AppendText(msg)

    def CDisplay(self):
	statusbar.SetStatusText('Connected.')
        self.taskBarIcon.SetIcon(wx.Icon(name='connected.png', type=wx.BITMAP_TYPE_PNG))
    def DDisplay(self):
	statusbar.SetStatusText('Disconnected.')
        self.taskBarIcon.SetIcon(wx.Icon(name='disconnected.png', type=wx.BITMAP_TYPE_PNG))

    #二次认证
    def Twice(self,username,password):
	count=0
	while True :
		if count >5 :
			wx.CallAfter(self.Display,"二次认证失败，请登录 http://172.16.18.4手动认证，或注销后重新认证。\n")
			break
		else :
			origURL="http://172.16.18.4"
			host='172.16.18.4'
			conn=httplib.HTTPConnection(host,80)
			headers={
			'Connection':'keep-alive',
			'Content-Type':'application/x-www-form-urlencoded'}
			post=urllib.urlencode({'DDDDD':username,'upass':password,'0MKKey':'%B5%C7%C2%BC+Login','v6ip':''})
			headers2={'Content-Type':'application/x-www-form-urlencoded'}
			try :
				conn.request(method='POST',url='/',body=post,headers=headers)
				html=conn.getresponse().read()
				conn.close()
				if  html.find("Congratulations")!=-1 :
					wx.CallAfter(self.Display,"二次认证成功。\n")
					break
			except Exception :
				count=count+1
				time.sleep(1)
    	
class ThreadCli(threading.Thread):
	def __init__(self,username,password,devicename,window):
		threading.Thread.__init__(self)
		self.username=username
		self.password=password
		self.devicename=devicename
		self.window=window
	def run(self):
		global flag
		p=subprocess.Popen(['./h3c_ouc','-u',self.username,'-p',self.password,'-n',self.devicename],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=False)
		while True:
			out = p.stdout.readline()
			if out == '' and p.poll() != None:
				break
			if out!= '':
				if out == "success\n" :
					wx.CallAfter(self.window.Display,"认证成功。\n")
					wx.CallAfter(self.window.CDisplay)
					wx.CallAfter(self.window.Display,"正在登录到 172.16.18.4 服务器......\n")
					wx.CallAfter(self.window.Twice,self.username,self.password)
				else :
					wx.CallAfter(self.window.Display,out)
					wx.CallAfter(self.window.DDisplay)
		if p.returncode == 2:
			wx.CallAfter(self.window.Display,out)
			wx.CallAfter(self.window.CDisplay)	
				
class MyApp(wx.App):
    def OnInit(self):
        frame = MyFrame(None,-1,'Net connect')
        frame.Show(True)
        return True

def main():					#Main function.
    app = MyApp(0)
    app.MainLoop()
    
if __name__ == '__main__':

    main()
