const bindings = require('bindings')
const twain = bindings("twain")

describe('twain', () => {
    it('Protocol Version"', () => {
        expect(twain.TWON_PROTOCOLMAJOR).toBe(2)
        expect(twain.TWON_PROTOCOLMINOR).toBe(4)
    })

    it("Country Code & Language Code", () => {
        expect(twain.TWCY_CHINA).toBe(86)
        expect(twain.TWLG_CHINESE).toBe(37)
    })

    it('Twain class"', async () => {
        const session = new twain.TwainSDK({
            productName: "productName!",
            productFamily: "productFamily!",
            manufacturer: "manufacturer!",
            version: {
                country: twain.TWCY_CHINA,
                language: twain.TWLG_CHINESE,
                majorNum: 1,
                minorNum: 1,
                info: "v0.0.1",
            }
        })
        expect(session.getState()).toBe(3)

        const sources = session.getDataSources()
        console.log(sources)
        expect(sources.length).toBeGreaterThan(0)
        
        // pSource
        const defaultSource = session.getDefaultSource()
        console.log(defaultSource);
        expect(defaultSource).toBeTruthy()

        session.setDefaultSource(sources[0])

        await session.openDataSource()
        const enumTest = session.getCapability(twain.ICAP_XFERMECH)         // Enum
        const oneValueTest = session.getCapability(twain.CAP_AUTHOR)  // one value
        const rangeTest = session.getCapability(twain.ICAP_JPEGQUALITY)        // range
        const arrayTest = session.getCapability(twain.CAP_SUPPORTEDCAPS)    // array
        expect(enumTest).toHaveProperty("currentIndex")
        expect(enumTest).toHaveProperty("defaultIndex")
        expect(enumTest).toHaveProperty("itemList")
        expect(rangeTest).toHaveProperty("minValue")
        expect(rangeTest).toHaveProperty("maxValue")
        expect(rangeTest).toHaveProperty("stepSize")
        expect(rangeTest).toHaveProperty("defaultValue")
        expect(rangeTest).toHaveProperty("currentValue")
        expect(Array.isArray(arrayTest)).toBeTruthy()
        expect(oneValueTest).toBeTruthy()

        const c0 = session.getCapability(twain.CAP_XFERCOUNT)
        const c1 = session.getCapability(twain.ICAP_PIXELTYPE)
        const c2 = session.getCapability(twain.ICAP_XFERMECH)
        const c3 = session.getCapability(twain.ICAP_IMAGEFILEFORMAT)
        const c4 = session.getCapability(twain.ICAP_COMPRESSION)
        const c5 = session.getCapability(twain.ICAP_UNITS)
        const c6 = session.getCapability(twain.ICAP_BITDEPTH)
        const c7 = session.getCapability(twain.ICAP_XRESOLUTION)
        const c8 = session.getCapability(twain.ICAP_YRESOLUTION)
        const c9 = session.getCapability(twain.ICAP_FRAMES)

        console.log("CAP_XFERCOUNT", c0)
        console.log("ICAP_PIXELTYPE", c1)
        console.log("ICAP_XFERMECH", c2)
        console.log("ICAP_IMAGEFILEFORMAT", c3)
        console.log("ICAP_COMPRESSION", c4)
        console.log("ICAP_UNITS", c5)
        console.log("ICAP_BITDEPTH", c6)
        console.log("ICAP_XRESOLUTION", c7)
        console.log("ICAP_YRESOLUTION", c8)
        console.log("ICAP_FRAMES", c9)

        session.setCallback()
        await session.enableDataSource()
        session.scan(twain.TWSX_FILE, "C:\\Users\\A11200321050133\\Documents\\imageName")
    })
})


describe('quick', () => {

    it('quick class"', async () => {
        const session = new twain.TwainSDK({
            productName: "productName!",
            productFamily: "productFamily!",
            manufacturer: "manufacturer!",
            version: {
                country: twain.TWCY_CHINA,
                language: twain.TWLG_CHINESE,
                majorNum: 1,
                minorNum: 1,
                info: "v0.0.1",
            }
        })
        
        // sources
        // const sources = session.getDataSources()
        // expect(Array.isArray(sources)).toBeTruthy()
        // source
        const defaultSource = session.getDefaultSource()
        console.log(defaultSource)

        // pSource
        // session.setDefaultSource(defaultSource)

        // pSource, sources
        session.openDataSource(defaultSource)

        // session.openDataSource(defaultSource)
        // source
        session.setCallback()

        // pSource
        // session.enableDataSource()

        // pSource 普通扫描
        session.scan(twain.TWSX_FILE,
               "C:\\Users\\A11200321050133\\Documents\\Scanned Documents\\imageFromScanner",
                (rcCode, data) => {
                    console.log('收到回调:',rcCode, data);
                },
            1 //第一次扫描时：startIdx=1；中途暂停了扫描，继续扫描时startIdx=lastId+1
        );
        // pSource 重新扫描
        session.rescan(twain.TWSX_FILE,
                       "C:\\Users\\A11200321050133\\Documents\\Scanned Documents\\imageFromScanner",
                        (rcCode, data) => { //回调函数
                            console.log('收到回调:',rcCode, data);
                        },
                        [1,3,5,6]//待重新扫描的 index
        );
        // pOneValue
        session.setCapability(cap, TWON_ONEVALUE, {"itemType":1,"value": 1});

        // pRangeValue
        session.setCapability(cap, TWON_RANGE, {"itemType": 1,"minValue":1 ,"maxValue":1 ,"stepSize":1 ,"defaultValue":1 ,"currentValue": 1});

        // pEnumValue
        session.setCapability(cap, TWON_ENUMERATION, {"itemType": 1,"defaultValue": 1,"currentValue": 1,"itemList":[1,2] });

        // pArrayValue
        session.setCapability(cap, TWON_ARRAY, {"itemType": 1,"itemList":[1,1,2,3]});//数组第一个元素为itemType，后面元素为数据值
    })
})
