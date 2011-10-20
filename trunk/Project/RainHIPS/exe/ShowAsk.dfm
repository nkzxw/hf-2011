object Form3: TForm3
  Left = 0
  Top = 0
  BorderIcons = []
  BorderStyle = bsDialog
  Caption = #25552#31034
  ClientHeight = 267
  ClientWidth = 260
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 8
    Width = 52
    Height = 13
    Caption = #21160#20316#23487#20027':'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label3: TLabel
    Left = 8
    Top = 71
    Width = 52
    Height = 13
    Caption = #36816#34892#31867#22411':'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label4: TLabel
    Left = 8
    Top = 26
    Width = 78
    Height = 13
    Caption = '                          '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label5: TLabel
    Left = 8
    Top = 122
    Width = 78
    Height = 13
    Caption = '                          '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label6: TLabel
    Left = 66
    Top = 71
    Width = 78
    Height = 13
    Caption = '                          '
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Label2: TLabel
    Left = 8
    Top = 103
    Width = 52
    Height = 13
    Caption = #36816#34892#30446#26631':'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
  end
  object Button1: TButton
    Left = 8
    Top = 177
    Width = 75
    Height = 25
    Caption = #21516#24847
    TabOrder = 0
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 152
    Top = 177
    Width = 75
    Height = 25
    Caption = #31105#27490
    TabOrder = 1
    OnClick = Button2Click
  end
  object CheckBox1: TCheckBox
    Left = 8
    Top = 217
    Width = 121
    Height = 17
    Caption = #21160#20316#23487#20027#21152#20837#35268#21017
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    TabOrder = 2
  end
  object CheckBox2: TCheckBox
    Left = 8
    Top = 240
    Width = 121
    Height = 17
    Caption = #36816#34892#30446#26631#21152#20837#35268#21017
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    TabOrder = 3
  end
  object spDynamicSkinForm1: TspDynamicSkinForm
    DisableSystemMenu = False
    PositionInMonitor = sppDefault
    UseFormCursorInNCArea = False
    MaxMenuItemsInWindow = 0
    ClientWidth = 0
    ClientHeight = 0
    HideCaptionButtons = False
    AlwaysShowInTray = False
    LogoBitMapTransparent = False
    AlwaysMinimizeToTray = False
    UseSkinFontInMenu = True
    ShowIcon = True
    MaximizeOnFullScreen = False
    ShowObjectHint = False
    UseDefaultObjectHint = True
    UseSkinCursors = False
    DefCaptionFont.Charset = DEFAULT_CHARSET
    DefCaptionFont.Color = clBtnText
    DefCaptionFont.Height = 14
    DefCaptionFont.Name = 'Arial'
    DefCaptionFont.Style = [fsBold]
    DefInActiveCaptionFont.Charset = DEFAULT_CHARSET
    DefInActiveCaptionFont.Color = clBtnShadow
    DefInActiveCaptionFont.Height = 14
    DefInActiveCaptionFont.Name = 'Arial'
    DefInActiveCaptionFont.Style = [fsBold]
    DefMenuItemHeight = 20
    DefMenuItemFont.Charset = DEFAULT_CHARSET
    DefMenuItemFont.Color = clWindowText
    DefMenuItemFont.Height = 14
    DefMenuItemFont.Name = 'Arial'
    DefMenuItemFont.Style = []
    UseDefaultSysMenu = True
    SupportNCArea = True
    AlphaBlendAnimation = False
    AlphaBlendValue = 235
    AlphaBlend = True
    MenusAlphaBlend = False
    MenusAlphaBlendAnimation = False
    MenusAlphaBlendValue = 200
    SkinData = Form1.spSkinData1
    MinHeight = 0
    MinWidth = 0
    MaxHeight = 0
    MaxWidth = 0
    Sizeable = False
    DraggAble = False
    Magnetic = False
    MagneticSize = 10
    BorderIcons = [biSystemMenu]
    Left = 224
    Top = 80
  end
  object Timer1: TTimer
    Interval = 100
    OnTimer = Timer1Timer
    Left = 184
    Top = 112
  end
  object dsaSkinAdapter1: TdsaSkinAdapter
    SkinData = Form1.spSkinData1
    AdapterType = dsaUseClasses
    TabsBGTransparent = False
    AutoAddNames = False
    AutoAddClasses = True
    LabelSubclass.Strings = (
      'tlabel'
      'ttntlabel')
    ScrollSubclass.Strings = (
      'tmemo'
      'ttntmemo'
      'tdbmemo'
      'tlistbox'
      'tdblistbox'
      'tchecklistbox'
      'ttntlistbox'
      'trichedit'
      'tdbrichedit'
      'ttreeview'
      'ttnttreeview'
      'tstringgrid'
      'tdrawgrid'
      'tadvgrid'
      'tdbadvgrid'
      'trichview'
      'trichviewedit'
      'tdbrichviewedit'
      'tdbrichview'
      'tprofgrid'
      'twwgrid'
      'twwdbgrid'
      'tdbgrid'
      'tdbgridex'
      'tscrollbox'
      'ttntdbrichedit'
      'ttntdbmemo'
      'tvirtualtreeview'
      'tvirtualstringtree'
      'tvirtualdrawtree')
    VScrollSkinDataName = 'vscrollbar'
    HScrollSkinDataName = 'hscrollbar'
    ButtonSubclass.Strings = (
      'tbutton'
      'ttntbutton'
      'tbitbtn'
      'ttntbitbtn')
    SpeedButtonSubclass.Strings = (
      'tspeedbutton'
      'ttntspeedbutton')
    CheckBoxSubclass.Strings = (
      'tcheckbox'
      'ttntcheckbox'
      'tdbcheckbox'
      'ttntdbcheckbox'
      'twwcheckbox')
    RadioButtonSubclass.Strings = (
      'tradiobutton'
      'ttntradiobutton')
    UpDownSubclass.Strings = (
      'tupdown'
      'ttntupdown')
    EditSubclass.Strings = (
      'tedit'
      'tdbedit'
      'ttntedit'
      'ttntdbedit'
      'tspinedit'
      'ttntspinedit')
    DateTimeSubclass.Strings = (
      'tdatetimepicker')
    ComboBoxSubclass.Strings = (
      'tcombobox'
      'ttntcombobox'
      'tdbcombobox'
      'ttntdbcombobox'
      'tdblookupcombobox')
    ListViewSubclass.Strings = (
      'tlistview'
      'ttntlistview')
    TabSubclass.Strings = (
      'tpagecontrol'
      'ttabcontrol'
      'ttntpagecontrol'
      'ttnttabcontrol')
    GroupBoxSubclass.Strings = (
      'tgroupbox'
      'ttntgroupbox'
      'tradiogroup'
      'tdbradiogroup'
      'ttntradiogroup'
      'ttntdbradiogroup')
    PanelSubclass.Strings = (
      'tpanel'
      'ttntpanel')
    Version = 'SkinAdapter Version 4.10'
    ButtonSkinDataName = 'resizebutton'
    SpeedButtonSkinDataName = 'resizetoolbutton'
    ButtonUseSkinSize = False
    SpeedButtonUseSkinSize = False
    PanelSkinDataName = 'panel'
    Left = 184
    Top = 80
  end
end
