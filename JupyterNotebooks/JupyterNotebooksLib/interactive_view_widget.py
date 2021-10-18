import qt, slicer
from ipycanvas import Canvas

class ViewInteractiveWidget(Canvas):
  """Remote controller for Slicer viewers.
  :param layoutLabel: specify view by label (displayed in the view's header in the layout, such as R, Y, G, 1)
  :param renderView: specify view by renderView object (ctkVTKRenderView).
  """

  def __init__(self, layoutLabel=None, renderView=None, **kwargs):
    from ipyevents import Event
    #import time

    super().__init__(**kwargs)

    # Find renderView from layoutLabel
    layoutManager = slicer.app.layoutManager()
    # Find it among 3D views
    if not renderView:
      for threeDViewIndex in range(layoutManager.threeDViewCount):
        threeDWidget = layoutManager.threeDWidget(threeDViewIndex)
        if (threeDWidget.mrmlViewNode().GetLayoutLabel() == layoutLabel) or (layoutLabel is None):
          renderView = threeDWidget.threeDView()
          break
    # Find it among slice views
    if not renderView:
      sliceViewNames = layoutManager.sliceViewNames()
      for sliceViewName in sliceViewNames:
        sliceWidget = layoutManager.sliceWidget(sliceViewName)
        if (sliceWidget.mrmlSliceNode().GetLayoutLabel() == layoutLabel) or (layoutLabel is None):
          renderView = sliceWidget.sliceView()
          break

    if not renderView:
      if layoutLabel:
        raise ValueError("renderView is not specified and view cannot be found by layout label "+layoutLabel)
      else:
        raise ValueError("renderView is not specified")

    self.renderView = renderView

    # Frame rate (1/renderDelay)
    self.lastRenderTime = 0
    self.quickRenderDelaySec = 0.1
    self.quickRenderDelaySecRange = [0.02, 2.0]
    self.adaptiveRenderDelay = True
    self.lastMouseMoveEvent = None

    # Quality vs performance
    self.compressionQuality = 50
    self.trackMouseMove = False  # refresh if mouse is just moving (not dragging)

    self.messageTimestampOffset = None

    # If not receiving new rendering request for 10ms then a render is requested
    self.fullRenderRequestTimer = qt.QTimer()
    self.fullRenderRequestTimer.setSingleShot(True)
    self.fullRenderRequestTimer.setInterval(500)
    self.fullRenderRequestTimer.connect('timeout()', self.fullRender)

    # If not receiving new rendering request for 10ms then a render is requested
    self.quickRenderRequestTimer = qt.QTimer()
    self.quickRenderRequestTimer.setSingleShot(True)
    self.quickRenderRequestTimer.setInterval(self.quickRenderDelaySec*1000)
    self.quickRenderRequestTimer.connect('timeout()', self.quickRender)

    # Get image size
    image = self.getImage()
    self.width=image.width
    self.height=image.height
    self.draw_image(image)

    self.interactor = self.renderView.interactorStyle().GetInteractor()

    self.dragging=False

    self.interactionEvents = Event()
    self.interactionEvents.source = self
    self.interactionEvents.watched_events = [
        'dragstart', 'mouseenter', 'mouseleave',
        'mousedown', 'mouseup', 'mousemove',
        #'wheel',  # commented out so that user can scroll through the notebook using mousewheel
        'keyup', 'keydown',
        'contextmenu' # prevent context menu from appearing on right-click
        ]
    #self.interactionEvents.msg_throttle = 1  # does not seem to have effect
    self.interactionEvents.prevent_default_action = True
    self.interactionEvents.on_dom_event(self.handleInteractionEvent)

    self.keyToSym = {
      'ArrowLeft': 'Left',
      'ArrowRight': 'Right',
      'ArrowUp': 'Up',
      'ArrowDown': 'Down',
      'BackSpace': 'BackSpace',
      'Tab': 'Tab',
      'Enter': 'Return',
      #'Shift': 'Shift_L',
      #'Control': 'Control_L',
      #'Alt': 'Alt_L',
      'CapsLock': 'Caps_Lock',
      'Escape': 'Escape',
      ' ': 'space',
      'PageUp': 'Prior',
      'PageDown': 'Next',
      'Home': 'Home',
      'End': 'End',
      'Delete': 'Delete',
      'Insert': 'Insert',
      '*': 'asterisk',
      '+': 'plus',
      '|': 'bar',
      '-': 'minus',
      '.': 'period',
      '/': 'slash',
      'F1': 'F1',
      'F2': 'F2',
      'F3': 'F3',
      'F4': 'F4',
      'F5': 'F5',
      'F6': 'F6',
      'F7': 'F7',
      'F8': 'F8',
      'F9': 'F9',
      'F10': 'F10',
      'F11': 'F11',
      'F12': 'F12'
      }

    # Errors are not displayed when a widget is displayed,
    # this variable can be used to retrieve error messages
    self.error = None

    # Enable logging of UI events
    self.logEvents = False
    self.loggedEvents = []
    self.elapsedTimes = []
    self.ageOfProcessedMessages = []

  def setQuickRenderDelay(self, delaySec):
    """Delay this much after a view update before sending a low-resolution update."""
    if delaySec<self.quickRenderDelaySecRange[0]:
        delaySec = self.quickRenderDelaySecRange[0]
    elif delaySec>self.quickRenderDelaySecRange[1]:
        delaySec = self.quickRenderDelaySecRange[1]
    self.quickRenderDelaySec = delaySec
    self.quickRenderRequestTimer.setInterval(self.quickRenderDelaySec*1000)

  def setFullRenderDelay(self, delaySec):
    """Delay this much after a view update before sending a full-resolution update."""
    self.fullRenderRequestTimer.setInterval(delaySec)

  def getImage(self, compress=True, forceRender=True):
    """Retrieve an image from the view."""
    from ipywidgets import Image
    slicer.app.processEvents()
    if forceRender:
      self.renderView.forceRender()
    screenshot = self.renderView.grab()
    bArray = qt.QByteArray()
    buffer = qt.QBuffer(bArray)
    buffer.open(qt.QIODevice.WriteOnly)
    if compress:
      screenshot.save(buffer, "JPG", self.compressionQuality)
    else:
      screenshot.save(buffer, "PNG")
    return Image(value=bArray.data(), width=screenshot.width(), height=screenshot.height())

  def fullRender(self):
    """Perform a full render now."""
    try:
      import time
      self.fullRenderRequestTimer.stop()
      self.quickRenderRequestTimer.stop()
      self.draw_image(self.getImage(compress=False, forceRender=True))
      self.lastRenderTime = time.time()
    except Exception as e:
      self.error = str(e)

  def sendPendingMouseMoveEvent(self):
    if self.lastMouseMoveEvent is not None:
      self.updateInteractorEventData(self.lastMouseMoveEvent)
      self.interactor.MouseMoveEvent()
      self.lastMouseMoveEvent = None

  def quickRender(self):
    """Perform a quick render now."""
    try:
      import time
      self.fullRenderRequestTimer.stop()
      self.quickRenderRequestTimer.stop()
      self.sendPendingMouseMoveEvent()
      self.draw_image(self.getImage(compress=True, forceRender=False))
      self.fullRenderRequestTimer.start()
      if self.logEvents: self.elapsedTimes.append(time.time()-self.lastRenderTime)
      self.lastRenderTime = time.time()
    except Exception as e:
      self.error = str(e)

  def updateInteractorEventData(self, event):
    try:
      if event['event']=='keydown' or event['event']=='keyup':
        key = event['key']
        sym = self.keyToSym[key] if key in self.keyToSym.keys() else key
        self.interactor.SetKeySym(sym)
        if len(key) == 1:
          self.interactor.SetKeyCode(key)
        self.interactor.SetRepeatCount(1)
      else:
        self.interactor.SetEventPosition(event['offsetX'], self.height-event['offsetY'])
      self.interactor.SetShiftKey(event['shiftKey'])
      self.interactor.SetControlKey(event['ctrlKey'])
      self.interactor.SetAltKey(event['altKey'])
    except Exception as e:
      self.error = str(e)

  def handleInteractionEvent(self, event):
    try:
      if self.logEvents:
        self.loggedEvents.append(event)
      if event['event']=='mousemove':
        import time
        if self.messageTimestampOffset is None:
            self.messageTimestampOffset = time.time()-event['timeStamp']*0.001
        self.lastMouseMoveEvent = event
        if not self.dragging and not self.trackMouseMove:
            return
        if self.adaptiveRenderDelay:
            ageOfProcessedMessage = time.time()-(event['timeStamp']*0.001+self.messageTimestampOffset)
            if ageOfProcessedMessage > 1.5 * self.quickRenderDelaySec:
                # we are falling behind, try to render less frequently
                self.setQuickRenderDelay(self.quickRenderDelaySec * 1.05)
            elif ageOfProcessedMessage < 0.5 * self.quickRenderDelaySec:
                # we can keep up with events, try to render more frequently
                self.setQuickRenderDelay(self.quickRenderDelaySec / 1.05)
            if self.logEvents: self.ageOfProcessedMessages.append([ageOfProcessedMessage, self.quickRenderDelaySec])
        # We need to render something now it no rendering since self.quickRenderDelaySec
        if time.time()-self.lastRenderTime > self.quickRenderDelaySec:
            self.quickRender()
        else:
            self.quickRenderRequestTimer.start()
      elif event['event']=='mouseenter':
        self.updateInteractorEventData(event)
        self.interactor.EnterEvent()
        self.lastMouseMoveEvent = None
        self.quickRenderRequestTimer.start()
      elif event['event']=='mouseleave':
        self.updateInteractorEventData(event)
        self.interactor.LeaveEvent()
        self.lastMouseMoveEvent = None
        self.quickRenderRequestTimer.start()
      elif event['event']=='mousedown':
        self.dragging=True
        self.sendPendingMouseMoveEvent()
        self.updateInteractorEventData(event)
        if event['button'] == 0:
          self.interactor.LeftButtonPressEvent()
        elif event['button'] == 2:
          self.interactor.RightButtonPressEvent()
        elif event['button'] == 1:
          self.interactor.MiddleButtonPressEvent()
        self.fullRender()
      elif event['event']=='mouseup':
        self.sendPendingMouseMoveEvent()
        self.updateInteractorEventData(event)
        if event['button'] == 0:
          self.interactor.LeftButtonReleaseEvent()
        elif event['button'] == 2:
          self.interactor.RightButtonReleaseEvent()
        elif event['button'] == 1:
          self.interactor.MiddleButtonReleaseEvent()
        self.dragging=False
        self.fullRender()
      elif event['event']=='keydown':
        self.sendPendingMouseMoveEvent()
        self.updateInteractorEventData(event)
        self.interactor.KeyPressEvent()
        self.interactor.CharEvent()
        if event['key'] != 'Shift' and event['key'] != 'Control' and event['key'] != 'Alt':
          self.fullRender()
      elif event['event']=='keyup':
        self.sendPendingMouseMoveEvent()
        self.updateInteractorEventData(event)
        self.interactor.KeyReleaseEvent()
        if event['key'] != 'Shift' and event['key'] != 'Control' and event['key'] != 'Alt':
          self.fullRender()
    except Exception as e:
        self.error = str(e)
