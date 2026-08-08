import yfinance as yf
import pandas as pd
from pathlib import Path


STOCKS = [
    # =========================
    # BANKING / FINANCE
    # =========================
    "HDFCBANK.NS",
    "ICICIBANK.NS",
    "SBIN.NS",
    "AXISBANK.NS",
    "KOTAKBANK.NS",
    "INDUSINDBK.NS",
    "BANKBARODA.NS",
    "PNB.NS",
    "CANBK.NS",
    "UNIONBANK.NS",
    "IDFCFIRSTB.NS",
    "FEDERALBNK.NS",
    "BANDHANBNK.NS",
    "AUROPHARMA.NS",
    "AUBANK.NS",

    "BAJFINANCE.NS",
    "BAJAJFINSV.NS",
    "SHRIRAMFIN.NS",
    "CHOLAFIN.NS",
    "MUTHOOTFIN.NS",
    "MANAPPURAM.NS",
    "LICHSGFIN.NS",
    "PFC.NS",
    "RECLTD.NS",
    "IRFC.NS",

    "HDFCLIFE.NS",
    "SBILIFE.NS",
    "ICICIPRULI.NS",
    "ICICIGI.NS",
    "SBICARD.NS",
    "HDFCAMC.NS",
    "LICI.NS",

    # =========================
    # IT / SOFTWARE
    # =========================
    "TCS.NS",
    "INFY.NS",
    "HCLTECH.NS",
    "WIPRO.NS",
    "TECHM.NS",
    "LTIM.NS",
    "COFORGE.NS",
    "PERSISTENT.NS",
    "MPHASIS.NS",
    "LTTS.NS",
    "TATAELXSI.NS",
    "OFSS.NS",
    "KPITTECH.NS",
    "CYIENT.NS",
    "TATATECH.NS",
    "MINDTREE.NS",

    # =========================
    # OIL / GAS / ENERGY
    # =========================
    "RELIANCE.NS",
    "ONGC.NS",
    "IOC.NS",
    "BPCL.NS",
    "HINDPETRO.NS",
    "GAIL.NS",
    "OIL.NS",
    "PETRONET.NS",
    "IGL.NS",
    "MGL.NS",
    "ATGL.NS",

    "NTPC.NS",
    "POWERGRID.NS",
    "TATAPOWER.NS",
    "JSWENERGY.NS",
    "TORNTPOWER.NS",
    "ADANIGREEN.NS",
    "ADANIPOWER.NS",
    "NHPC.NS",
    "SJVN.NS",

    # =========================
    # AUTOMOBILE
    # =========================
    "MARUTI.NS",
    "TATAMOTORS.NS",
    "M&M.NS",
    "BAJAJ-AUTO.NS",
    "EICHERMOT.NS",
    "HEROMOTOCO.NS",
    "TVSMOTOR.NS",
    "ASHOKLEY.NS",
    "BOSCHLTD.NS",
    "BHARATFORG.NS",
    "MOTHERSON.NS",
    "EXIDEIND.NS",
    "MRF.NS",
    "APOLLOTYRE.NS",
    "ESCORTS.NS",
    "TIINDIA.NS",

    # =========================
    # PHARMA / HEALTHCARE
    # =========================
    "SUNPHARMA.NS",
    "DRREDDY.NS",
    "CIPLA.NS",
    "DIVISLAB.NS",
    "APOLLOHOSP.NS",
    "LUPIN.NS",
    "AUROPHARMA.NS",
    "ZYDUSLIFE.NS",
    "TORNTPHARM.NS",
    "ALKEM.NS",
    "BIOCON.NS",
    "MAXHEALTH.NS",
    "FORTIS.NS",
    "LAURUSLABS.NS",
    "GLENMARK.NS",
    "IPCALAB.NS",
    "ABBOTINDIA.NS",
    "AJANTPHARM.NS",
    "MANKIND.NS",

    # =========================
    # METALS / MINING
    # =========================
    "TATASTEEL.NS",
    "JSWSTEEL.NS",
    "HINDALCO.NS",
    "VEDL.NS",
    "HINDZINC.NS",
    "JINDALSTEL.NS",
    "SAIL.NS",
    "NMDC.NS",
    "COALINDIA.NS",
    "NATIONALUM.NS",
    "APLAPOLLO.NS",

    # =========================
    # CONSTRUCTION / CEMENT
    # =========================
    "LT.NS",
    "ULTRACEMCO.NS",
    "GRASIM.NS",
    "AMBUJACEM.NS",
    "ACC.NS",
    "SHREECEM.NS",
    "DALBHARAT.NS",
    "JKCEMENT.NS",
    "RAMCOCEM.NS",
    "JKLAKSHMI.NS",

    # =========================
    # CAPITAL GOODS / ENGINEERING
    # =========================
    "SIEMENS.NS",
    "ABB.NS",
    "BEL.NS",
    "HAL.NS",
    "BHEL.NS",
    "RVNL.NS",
    "IRCON.NS",
    "BDL.NS",
    "BEML.NS",
    "CUMMINSIND.NS",
    "THERMAX.NS",
    "POLYCAB.NS",
    "HAVELLS.NS",
    "VOLTAS.NS",
    "KEI.NS",
    "CGPOWER.NS",
    "SUZLON.NS",
    "KAYNES.NS",

    # =========================
    # CONSUMER / FMCG
    # =========================
    "HINDUNILVR.NS",
    "ITC.NS",
    "NESTLEIND.NS",
    "BRITANNIA.NS",
    "TATACONSUM.NS",
    "DABUR.NS",
    "MARICO.NS",
    "GODREJCP.NS",
    "COLPAL.NS",
    "UBL.NS",
    "UNITEDSPIRITS.NS",
    "VBL.NS",
    "JUBLFOOD.NS",
    "DMART.NS",
    "TRENT.NS",
    "TITAN.NS",
    "KALYANKJIL.NS",
    "PAGEIND.NS",
    "BATAINDIA.NS",

    # =========================
    # TELECOM / MEDIA
    # =========================
    "BHARTIARTL.NS",
    "IDEA.NS",
    "INDUSTOWER.NS",
    "BHARTIHEXA.NS",
    "ZEEL.NS",
    "SUNTV.NS",
    "PVRINOX.NS",

    # =========================
    # REAL ESTATE
    # =========================
    "DLF.NS",
    "LODHA.NS",
    "GODREJPROP.NS",
    "OBEROIRLTY.NS",
    "PHOENIXLTD.NS",
    "PRESTIGE.NS",
    "SOBHA.NS",
    "BRIGADE.NS",

    # =========================
    # INFRA / TRANSPORT
    # =========================
    "ADANIPORTS.NS",
    "ADANIENT.NS",
    "IRCTC.NS",
    "CONCOR.NS",
    "DELHIVERY.NS",
    "GMRINFRA.NS",
    "INDIGO.NS",
    "IRB.NS",
    "ASHOKA.NS",
    "KEC.NS",

    # =========================
    # CHEMICALS
    # =========================
    "PIDILITIND.NS",
    "SRF.NS",
    "UPL.NS",
    "DEEPAKNTR.NS",
    "AARTIIND.NS",
    "NAVINFLUOR.NS",
    "ATUL.NS",
    "ALKYLAMINE.NS",
    "PIIND.NS",
    "COROMANDEL.NS",
    "SUMICHEM.NS",

    # =========================
    # TEXTILES / APPAREL
    # =========================
    "PAGEIND.NS",
    "KPRMILL.NS",
    "WELSPUNLIV.NS",
    "TRIDENT.NS",
    "ARVIND.NS",
    "RAYMOND.NS",

    # =========================
    # SPECIAL / DIVERSIFIED
    # =========================
    "ADANIENT.NS",
    "ADANIPORTS.NS",
    "ADANIGREEN.NS",
    "ADANIPOWER.NS",
    "ADANITOTALGAS.NS",
    "TATACONSUM.NS",
    "TATACOMM.NS",
    "TATAINVEST.NS",
    "TATACHEM.NS",
    "TATASTEEL.NS",

    # =========================
    # OTHER POPULAR STOCKS
    # =========================
    "INDIANB.NS",
    "RBLBANK.NS",
    "IDBI.NS",
    "YESBANK.NS",
    "UCOBANK.NS",
    "MAHABANK.NS",

    "ZOMATO.NS",
    "NYKAA.NS",
    "PAYTM.NS",
    "POLICYBZR.NS",
    "DELHIVERY.NS",

    "INOXWIND.NS",
    "INOXGREEN.NS",
    "IREDA.NS",
    "HUDCO.NS",
    "JIOFIN.NS",

    "MCX.NS",
    "CDSL.NS",
    "BSE.NS",
    "ANGELONE.NS",
    "IIFLSEC.NS",
    "MOTILALOFS.NS",
    "360ONE.NS",

    "DIXON.NS",
    "KAYNES.NS",
    "IDEAFORGE.NS",
    "NETWEB.NS",
    "OLECTRA.NS",

    "ASTRAL.NS",
    "SUPREMEIND.NS",
    "KEI.NS",
    "FINOLEXIND.NS",
    "APLAPOLLO.NS",

    "MOTHERSON.NS",
    "ENDURANCE.NS",
    "SONACOMS.NS",
    "CRAFTSMAN.NS",
    "UNO-MINDA.NS",

    "BALKRISIND.NS",
    "SCHAEFFLER.NS",
    "SKFINDIA.NS",

    "LAURUSLABS.NS",
    "ERIS.NS",
    "GLAND.NS",
    "MEDANTA.NS",
    "JUBLPHARMA.NS",

    "INDHOTEL.NS",
    "LEMONTREE.NS",
    "CHALET.NS",

    "HINDCOPPER.NS",
    "MOIL.NS",
    "RATNAMANI.NS",

    "CANFINHOME.NS",
    "LICHSGFIN.NS",
    "ABCAPITAL.NS",
    "M&MFIN.NS",
    "POONAWALLA.NS",

    "FORTIS.NS",
    "METROPOLIS.NS",
    "LALPATHLAB.NS",

    "CROMPTON.NS",
    "WHIRLPOOL.NS",
    "BLUESTARCO.NS",
    "DIXON.NS",

    "BIRLACORPN.NS",
    "HEIDELBERG.NS",
    "JKPAPER.NS",

    "GUJGASLTD.NS",
    "AEGISLOG.NS",
    "PETRONET.NS",

    "CANBK.NS",
    "BANKINDIA.NS",
    "CENTRALBK.NS",
    "IOB.NS",

    "POWERMECH.NS",
    "KNRCON.NS",
    "PNCINFRA.NS",
    "NBCC.NS",

    "CESC.NS",
    "TORNTPOWER.NS",
    "NHPC.NS",
    "SJVN.NS",

    "MGL.NS",
    "IGL.NS",
    "GSPL.NS",

    "FACT.NS",
    "GNFC.NS",
    "RCF.NS",
]


output_dir = Path("../algorithm/data")
output_dir.mkdir(parents=True, exist_ok=True)

all_data = []


for ticker in STOCKS:

    print(f"Downloading {ticker}...")

    try:

        data = yf.download(
            ticker,
            period="10y",
            interval="1d",
            auto_adjust=True,
            progress=False
        )

        if data.empty:
            print(f"No data found for {ticker}")
            continue

        data = data.reset_index()

        if isinstance(data.columns, pd.MultiIndex):
            data.columns = data.columns.get_level_values(0)

        data = data[
            ["Date", "Open", "High", "Low", "Close", "Volume"]
        ]

        data["Symbol"] = ticker.replace(".NS", "")

        data = data[
            [
                "Symbol",
                "Date",
                "Open",
                "High",
                "Low",
                "Close",
                "Volume"
            ]
        ]

        all_data.append(data)

        print(f"Downloaded {len(data)} rows")


    except Exception as e:

        print(f"Error downloading {ticker}: {e}")



if all_data:

    final_data = pd.concat(
        all_data,
        ignore_index=True
    )

    final_data["Date"] = pd.to_datetime(
        final_data["Date"]
    ).dt.strftime("%Y-%m-%d")

    final_data = final_data.sort_values(
        ["Symbol", "Date"]
    )

    output_file = output_dir / "stocks.csv"

    final_data.to_csv(
        output_file,
        index=False
    )

    print()
    print("==============================")
    print("Download complete!")
    print("==============================")
    print(f"Stocks: {final_data['Symbol'].nunique()}")
    print(f"Rows: {len(final_data)}")
    print(f"Saved to: {output_file}")

else:

    print("No data was downloaded.")