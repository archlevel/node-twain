//
// Created by Lossa on 2022/7/21.
//

#include "TwainSDK.h"
#include "napi.h"

TwainSDK::TwainSDK(const Napi::CallbackInfo &info) : Napi::ObjectWrap<TwainSDK>(info) {
    Napi::Object configure = info[0].As<Napi::Object>();

    Napi::Object version = configure.Get("version").As<Napi::Object>();

    Napi::Number versionCountry = version.Get("country").As<Napi::Number>();
    Napi::Number versionLanguage = version.Get("language").As<Napi::Number>();
    Napi::Number versionMajorNum = version.Get("majorNum").As<Napi::Number>();
    Napi::Number versionMinorNum = version.Get("minorNum").As<Napi::Number>();
    Napi::String versionInfo = version.Get("info").As<Napi::String>();
    Napi::String productName = configure.Get("productName").As<Napi::String>();
    Napi::String productFamily = configure.Get("productFamily").As<Napi::String>();
    Napi::String manufacturer = configure.Get("manufacturer").As<Napi::String>();

    TW_IDENTITY identity;
    identity.Id = 0;
    identity.Version.Country = versionCountry.Int32Value();
    identity.Version.Language = versionLanguage.Int32Value();
    identity.Version.MajorNum = versionMajorNum.Int32Value();
    identity.Version.MinorNum = versionMinorNum.Int32Value();
    strcpy((char *) identity.Version.Info, versionInfo.Utf8Value().c_str());
    strcpy((char *) identity.ProductName, productName.Utf8Value().c_str());
    strcpy((char *) identity.ProductFamily, productFamily.Utf8Value().c_str());
    strcpy((char *) identity.Manufacturer, manufacturer.Utf8Value().c_str());
    identity.SupportedGroups = DF_APP2 | DG_IMAGE | DG_CONTROL;
    identity.ProtocolMajor = TWON_PROTOCOLMAJOR;
    identity.ProtocolMinor = TWON_PROTOCOLMINOR;

    session.fillIdentity(identity);

    session.loadDSM();     // state 1 -> state 2
    session.openDSM();     // state 2 -> state 3

    session.getSources();   // sources init
    session.getDefaultDS(); // source init
    Napi::String defaultName = Napi::String::New(info.Env(), reinterpret_cast<char *>(session.source.ProductName));
    session.setDefaultDS(defaultName); // pSource init
}

TwainSDK::~TwainSDK() {
    session.closeDS();
    session.freeDSM();
}

Napi::Value TwainSDK::getState(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    return Napi::Number::New(env, session.state);
}

Napi::Value TwainSDK::getDataSources(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    session.getSources();
    std::cout << "Trigger getDataSources" << std::endl;
    uint32_t i = 0;
    Napi::Array array = Napi::Array::New(env, session.sources.size());
    for (auto &&it: session.sources) {
        array[i++] = Napi::String::New(env, reinterpret_cast<char *>(it.ProductName));
    }
    return array;
}

Napi::Value TwainSDK::getDefaultSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    TW_UINT16 rc = session.getDefaultDS();
    Napi::String str = Napi::String::New(env, "");
    if(rc == TWRC_SUCCESS) {
        str = Napi::String::New(env, reinterpret_cast<char *>(session.source.ProductName));
    }
    return str;
}

Napi::Value TwainSDK::setDefaultSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if(info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string productName = info[0].As<Napi::String>().Utf8Value();

    TW_UINT16 rc = session.setDefaultDS(productName);

    return Napi::Boolean::New(env, rc == TWRC_SUCCESS);
}

Napi::Value TwainSDK::openDataSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    
    TW_UINT16 rc = TWRC_FAILURE;
    if (info.Length() < 1) {
        rc = session.openDS();
    } else {
        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
            return env.Null();
        }
        std::string productName = info[0].As<Napi::String>().Utf8Value();
        rc = session.openDS(productName);
    }
   
    return Napi::Boolean::New(env, rc == TWRC_SUCCESS);
}

Napi::Value TwainSDK::setCallback(const Napi::CallbackInfo &info) {
    Napi::Function jsFunction;
    Napi::Env env = info.Env();

    if (info.Length() > 0) {
        jsFunction = info[0].As<Napi::Function>();
    }
    session.setCallback(env,jsFunction);
    return Napi::Boolean::New(env, true);
}

Napi::Value TwainSDK::getCapability(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected a number").ThrowAsJavaScriptException();
        return env.Null();
    }

    TW_UINT16 CAP = info[0].As<Napi::Number>().Uint32Value();

    TW_CAPABILITY cap;
    cap.Cap = CAP;
    cap.hContainer = 0;
    TW_UINT16 rc = session.getCap(cap);
// The following structures combinations are implimented and found in the TWAIN specifications
//              BOOL  INT8  INT16  INT32  UINT8  UINT16  UINT32  STR32  STR64  STR128  STR255  STR1024  UNI512  FIX32  FRAME
// OneValue      x           x      x             x       x       x             x       x                        x      x
// Array                                   x      x       x       x                                              x      x
// Enumeration   x           x                    x       x       x                     x                        x      x
// Range                     x      x             x       x                                                      x
    if (rc != TWRC_SUCCESS) {
        return Napi::Boolean::New(env, false);
    }
    if (cap.ConType == TWON_RANGE) {
        std::cout << "get ConType TWON_RANGE" << std::endl;
        pTW_RANGE pRange = (pTW_RANGE) session.lockMemory(cap.hContainer);
        Napi::Object rangeResult = Napi::Object::New(env);
        std::cout << "get pRange->ItemType= "<< pRange->ItemType << std::endl;
        switch (pRange->ItemType) {
            case TWTY_INT8:
            case TWTY_INT16:
            case TWTY_INT32:
            case TWTY_UINT8:
            case TWTY_UINT16:
            case TWTY_UINT32:
            case TWTY_FIX32:
                rangeResult.Set("minValue", Napi::Number::New(env, pRange->MinValue));
                rangeResult.Set("maxValue", Napi::Number::New(env, pRange->MaxValue));
                rangeResult.Set("stepSize", Napi::Number::New(env, pRange->StepSize));
                rangeResult.Set("defaultValue", Napi::Number::New(env, pRange->DefaultValue));
                rangeResult.Set("currentValue", Napi::Number::New(env, pRange->CurrentValue));
                break;
        }
        session.unlockMemory(cap.hContainer);
        return rangeResult;
    } else if (cap.ConType == TWON_ARRAY) {
        std::cout << "get ConType TWON_ARRAY" << std::endl;
        pTW_ARRAY pArray = (pTW_ARRAY) session.lockMemory(cap.hContainer);
        Napi::Array arr = Napi::Array::New(env, pArray->NumItems);
        for (TW_UINT32 index = 0; index < pArray->NumItems; index++) {
            std::cout << "get index= "<< index << ", pArray->ItemType= " << pArray->ItemType << "pArray->ItemList" << pArray->ItemList[index] << std::endl;
            switch (pArray->ItemType) {
                case TWTY_INT8:
                case TWTY_INT16:
                case TWTY_INT32:
                case TWTY_UINT8:
                case TWTY_UINT16:
                case TWTY_UINT32:
                case TWTY_FIX32:
                    arr[index] = Napi::Number::New(env, pArray->ItemList[index]);
                    break;
                case TWTY_BOOL:
                    arr[index] = Napi::Boolean::New(env, pArray->ItemList[index] == 1 ? true : false);
                    break;
                default:
                    std::cerr << "Unsupported item type in TWON_ARRAY" << std::endl;
                    session.unlockMemory(cap.hContainer);
                    return Napi::Boolean::New(env, false);
            }
        }
        session.unlockMemory(cap.hContainer);
        return arr;
    } else if (cap.ConType == TWON_ONEVALUE) {
        std::cout << "get ConType TWON_ONEVALUE" << std::endl;
        pTW_ONEVALUE pOne = (pTW_ONEVALUE) session.lockMemory(cap.hContainer);
        std::cout << "get pOne->ItemType= "<< pOne->ItemType << std::endl;
        Napi::Value result;
        switch (pOne->ItemType) {
            case TWTY_INT8:
            case TWTY_INT16:
            case TWTY_INT32:
            case TWTY_UINT8:
            case TWTY_UINT16:
            case TWTY_UINT32:
            case TWTY_FIX32:
                result = Napi::Number::New(env, pOne->Item);
                break;
            case TWTY_BOOL:
                result = Napi::Boolean::New(env, (TW_BOOL)pOne->Item == 1 ? true : false);  
                break;
            case TWTY_STR32: {
                pTW_STR32 str32 = ((pTW_STR32) (&pOne->Item));
                result =  Napi::String::New(env, reinterpret_cast<char *>(str32));
                break;
            }
            case TWTY_STR64: {
                pTW_STR64 str64 = ((pTW_STR64) (&pOne->Item));
                result =  Napi::String::New(env, reinterpret_cast<char *>(str64));
                break;
            }
            case TWTY_STR128: {
                pTW_STR128 str128 = ((pTW_STR128) (&pOne->Item));
                result =  Napi::String::New(env, reinterpret_cast<char *>(str128));
                break;
            }
            case TWTY_STR255: {
                pTW_STR255 str255 = ((pTW_STR255) (&pOne->Item));
                result =  Napi::String::New(env, reinterpret_cast<char *>(str255));
                break;
            }
        }
        session.unlockMemory(cap.hContainer);
        return result;
    } else if (cap.ConType == TWON_ENUMERATION) {
        std::cout << "get ConType TWON_ENUMERATION" << std::endl;
        pTW_ENUMERATION pEnum = (pTW_ENUMERATION) session.lockMemory(cap.hContainer);
        Napi::Object enumResult = Napi::Object::New(env);
        Napi::Array list = Napi::Array::New(env, pEnum->NumItems);
        std::cout << "get pEnum->ItemType= "<< pEnum->ItemType << std::endl;
        for (TW_UINT32 index = 0; index < pEnum->NumItems; index++){
            std::cout << "get index= " << index << ", pEnum->ItemType= " << pEnum->ItemType << "pEnum->ItemList" << pEnum->ItemList[index] << std::endl;
            switch (pEnum->ItemType) {
                case TWTY_INT8:
                    list[index] = ((pTW_INT8)(&pEnum->ItemList))[index];
                    break;
                case TWTY_INT16:
                    list[index] = ((pTW_INT16)(&pEnum->ItemList))[index];
                    break;
                case TWTY_INT32:
                    list[index] = ((pTW_INT32)(&pEnum->ItemList))[index];
                    break;
                case TWTY_UINT8:
                    list[index] = ((pTW_UINT8)(&pEnum->ItemList))[index];
                    break;
                case TWTY_UINT16:
                    list[index] = ((pTW_UINT16)(&pEnum->ItemList))[index];
                    break;
                case TWTY_UINT32:
                    list[index] = ((pTW_UINT32)(&pEnum->ItemList))[index];
                    break;
                case TWTY_FIX32:
                    list[index] = ((pTW_UINT32)(&pEnum->ItemList))[index];
                    break;
                case TWTY_BOOL:
                    list[index] = ((pTW_BOOL)(&pEnum->ItemList))[index];
                    break;
           }
        }

        enumResult.Set("currentIndex", pEnum->CurrentIndex);
        enumResult.Set("defaultIndex", pEnum->DefaultIndex);
        enumResult.Set("itemList", list);
        session.unlockMemory(cap.hContainer);
        return enumResult;
    }
    return Napi::Boolean::New(env, false);
}

Napi::Value TwainSDK::setCapability(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    //if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber()) {
        //Napi::TypeError::New(env, "Expected three numbers").ThrowAsJavaScriptException();
        //return env.Null();
   //}
    std::cout << "setCapability:" << "-aa" << std::endl;
    TW_UINT16 CAP = static_cast<TW_UINT16>(info[0].As<Napi::Number>().Uint32Value());
    std::cout << "setCapability:" << "-bb" << std::endl;
    TW_UINT16 ITEM_TYPE = static_cast<TW_UINT16>(info[1].As<Napi::Number>().Uint32Value());
    std::cout << "setCapability:" << "-cc" << std::endl;
    Napi::Value value = info[2];
    std::cout << "setCapability:" << "-dd" << std::endl;
    TW_UINT16 rc = session.setCap(CAP, ITEM_TYPE,value);
    if (rc == TWRC_SUCCESS) {
        return Napi::Boolean::New(env, true);
    } else {
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value TwainSDK::enableDataSource(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    TW_UINT16 rc = session.enableDS();
    if (rc == TWRC_SUCCESS) {
        deferred.Resolve(Napi::String::New(info.Env(), "OK"));
    } else {
        deferred.Reject(Napi::String::New(info.Env(), "Reject"));
    }
    return deferred.Promise();
}

Napi::Value TwainSDK::scan(const Napi::CallbackInfo &info) {
    Napi::Function jsFunction;
    Napi::Env env = info.Env();
    TW_UINT16 transfer = info[0].As<Napi::Number>().Uint32Value();
    std::string path = info[1].As<Napi::String>().Utf8Value();
    if (info.Length() > 2) {
        jsFunction = info[2].As<Napi::Function>();
    }

    session.enableDS();
    session.scan(transfer, path, env, jsFunction);
    session.disableDS();
    return Napi::Boolean::New(env, true);
}

Napi::Value TwainSDK::release(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  session.closeDS();
  session.freeDSM();
  return Napi::Boolean::New(env, false);
}



