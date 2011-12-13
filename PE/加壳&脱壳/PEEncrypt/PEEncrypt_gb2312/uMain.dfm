object MainFrm: TMainFrm
  Left = 245
  Top = 156
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'PE Encrypt v1.0 :: Delphi design'
  ClientHeight = 238
  ClientWidth = 389
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 12
  object GroupBox1: TGroupBox
    Left = 10
    Top = 10
    Width = 371
    Height = 154
    Caption = #21152#23494#36873#39033
    TabOrder = 0
    object Label1: TLabel
      Left = 13
      Top = 35
      Width = 72
      Height = 12
      Caption = #25991' '#20214' '#21517'(&F):'
      FocusControl = EdtFileName
    end
    object Label2: TLabel
      Left = 13
      Top = 63
      Width = 72
      Height = 12
      Caption = #23494#12288#12288#30721'(&P):'
      FocusControl = EdtPwd
    end
    object Label3: TLabel
      Left = 13
      Top = 90
      Width = 72
      Height = 12
      Caption = #30830#35748#23494#30721'(&R):'
      FocusControl = EdtRptPwd
    end
    object EdtFileName: TEdit
      Left = 110
      Top = 30
      Width = 211
      Height = 20
      TabOrder = 0
    end
    object EdtPwd: TMaskEdit
      Left = 110
      Top = 58
      Width = 211
      Height = 20
      PasswordChar = '*'
      TabOrder = 1
    end
    object EdtRptPwd: TMaskEdit
      Left = 110
      Top = 85
      Width = 211
      Height = 20
      PasswordChar = '*'
      TabOrder = 2
    end
    object ChxBackup: TCheckBox
      Left = 15
      Top = 120
      Width = 244
      Height = 21
      Caption = #21152#23494#21069#22791#20221#25991#20214'(.bak)'
      Checked = True
      State = cbChecked
      TabOrder = 3
    end
    object BtnBrowse: TButton
      Left = 325
      Top = 30
      Width = 28
      Height = 23
      Caption = '..'
      TabOrder = 4
      OnClick = BtnBrowseClick
    end
  end
  object BtnEncrypt: TButton
    Left = 40
    Top = 190
    Width = 94
    Height = 29
    Caption = #21152#23494'(&E)'
    TabOrder = 1
    OnClick = BtnEncryptClick
  end
  object BtnAbout: TButton
    Left = 150
    Top = 190
    Width = 94
    Height = 29
    Caption = #20851#26044'(&A)'
    TabOrder = 2
    OnClick = BtnAboutClick
  end
  object BtnClose: TButton
    Left = 260
    Top = 190
    Width = 94
    Height = 29
    Caption = #36864#20986'(&X)'
    TabOrder = 3
    OnClick = BtnCloseClick
  end
end
