object MainFrm: TMainFrm
  Left = 245
  Top = 156
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'PE Encrypt v1.0 :: Delphi design'
  ClientHeight = 190
  ClientWidth = 311
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #32048#26126#39636
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 12
  object GroupBox1: TGroupBox
    Left = 8
    Top = 8
    Width = 297
    Height = 123
    Caption = #21152#23494#36984#38917
    TabOrder = 0
    object Label1: TLabel
      Left = 10
      Top = 28
      Width = 72
      Height = 12
      Caption = #25991' '#20214' '#21517'(&F):'
      FocusControl = EdtFileName
    end
    object Label2: TLabel
      Left = 10
      Top = 50
      Width = 72
      Height = 12
      Caption = #23494#12288#12288#30908'(&P):'
      FocusControl = EdtPwd
    end
    object Label3: TLabel
      Left = 10
      Top = 72
      Width = 72
      Height = 12
      Caption = #30906#35469#23494#30908'(&R):'
      FocusControl = EdtRptPwd
    end
    object EdtFileName: TEdit
      Left = 88
      Top = 24
      Width = 169
      Height = 20
      TabOrder = 0
    end
    object EdtPwd: TMaskEdit
      Left = 88
      Top = 46
      Width = 169
      Height = 20
      PasswordChar = '*'
      TabOrder = 1
    end
    object EdtRptPwd: TMaskEdit
      Left = 88
      Top = 68
      Width = 169
      Height = 20
      PasswordChar = '*'
      TabOrder = 2
    end
    object ChxBackup: TCheckBox
      Left = 12
      Top = 96
      Width = 195
      Height = 17
      Caption = #21152#23494#21069#20633#20221#25991#20214'(.bak)'
      Checked = True
      State = cbChecked
      TabOrder = 3
    end
    object BtnBrowse: TButton
      Left = 260
      Top = 24
      Width = 22
      Height = 18
      Caption = '..'
      TabOrder = 4
      OnClick = BtnBrowseClick
    end
  end
  object BtnEncrypt: TButton
    Left = 32
    Top = 152
    Width = 75
    Height = 23
    Caption = #21152#23494'(&E)'
    TabOrder = 1
    OnClick = BtnEncryptClick
  end
  object BtnAbout: TButton
    Left = 120
    Top = 152
    Width = 75
    Height = 23
    Caption = #38364#26044'(&A)'
    TabOrder = 2
    OnClick = BtnAboutClick
  end
  object BtnClose: TButton
    Left = 208
    Top = 152
    Width = 75
    Height = 23
    Caption = #36864#20986'(&X)'
    TabOrder = 3
    OnClick = BtnCloseClick
  end
end
